#![feature(ptr_as_ref_unchecked)]
#![feature(allocator_api)]
#![feature(slice_ptr_get)]

use core::{slice, str};
use elasticsearch::*;
use http::{
    request::Body,
    response::Response,
    transport::{SingleNodeConnectionPool, TransportBuilder},
    Url,
};
use log::{error, info, warn};
use serde::{de::Visitor, Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::{
    alloc::Layout,
    ffi::{c_char, CString},
    future::Future,
    ptr::null_mut,
    str::FromStr,
};

pub const TASK1_INDEX: &str = "task1_factory";
pub const TASK1_MAPPINGS: &str = r#"

"#;
pub const TASK2_INDEX: &str = "task2_library";
pub mod task_1;

#[inline]
fn wait4<T: Future>(f: T) -> T::Output {
    let rt = tokio::runtime::Runtime::new().unwrap();
    rt.block_on(f)
}

#[derive(Clone, Debug)]
#[repr(transparent)]
pub struct BufferString(pub [c_char; 80]);

impl Serialize for BufferString {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        let mut strlen = 0;
        while self.0[strlen] != 0 {
            strlen += 1;
        }
        // TODO: Print log warning and trim the string to valid len
        let s = unsafe { str::from_utf8(slice::from_raw_parts(self.0.as_ptr().cast(), strlen)) }
            .expect("Invalid Utf-8 string!");
        serializer.serialize_str(s)
    }
}

struct BufStrVisitor;
impl Visitor<'_> for BufStrVisitor {
    type Value = BufferString;

    fn expecting(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(formatter, "a valid utf-8 80-byte string")
    }

    fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
    where
        E: serde::de::Error,
    {
        let cstr = CString::from_str(v).expect("valid C UTF8 string");
        let v = cstr.to_bytes();
        if v.len() > 80 {
            Err(E::custom("too long string"))
        } else {
            let mut val = BufferString([0; 80]);
            let v = unsafe { slice::from_raw_parts(v.as_ptr().cast(), v.len()) };
            val.0[..v.len()].clone_from_slice(v);
            Ok(val)
        }
    }
}

impl<'de> Deserialize<'de> for BufferString {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        deserializer.deserialize_string(BufStrVisitor)
    }
}

pub trait ElasticId {
    fn get_id(&self) -> Option<&str>;
    fn set_id(&mut self, id: String);
}

#[derive(Deserialize)]
pub struct BulkResult {
    pub errors: bool,
    pub took: i32,
    items: Vec<JsonValue>,
}

impl BulkResult {
    pub fn is_ok(&self) -> bool {
        !self.errors
    }
    pub fn items(&self) -> &Vec<JsonValue> {
        &self.items
    }
    pub fn count(&self) -> usize {
        self.items.len()
    }
    /// Sets item's id's to those are in response.
    /// # Panics
    /// * If count of items retrieved from the server is not equal to items.len()
    /// * If response structure is invalid or it has `errors = true`
    pub fn write_ids<T: ElasticId>(&self, items: &mut [T]) {
        assert_eq!(self.items.len(), items.len(), "Different length");
        assert!(self.is_ok(), "Request failed");
        for (id, item) in self.items.iter().zip(items) {
            item.set_id(id["create"]["_id"].as_str().unwrap().to_owned());
        }
    }
}

#[derive(Deserialize, Debug)]
pub struct SearchHits {
    pub total: JsonValue,
    pub max_score: f32,
    pub hits: Vec<JsonValue>,
}

#[derive(Deserialize, Debug)]
pub struct SearchResult {
    pub took: i32,
    pub timed_out: bool,
    pub hits: SearchHits,
}

impl SearchResult {
    pub fn count(&self) -> usize {
        self.hits.total["value"].as_u64().unwrap() as usize
    }
    pub fn max_score(&self) -> f32 {
        self.hits.max_score
    }
    pub unsafe fn populate_array<T: for<'a> Deserialize<'a> + ElasticId>(
        self,
        items: *mut T,
        count: i32,
    ) {
        assert!(!self.timed_out, "Request is timed out");
        assert_eq!(
            self.count(),
            count as usize,
            "Length of results and slice are not equal"
        );
        for (i, mut res) in self.hits.hits.into_iter().enumerate() {
            items
                .add(i)
                .write(serde_json::from_value(res["_source"].take()).unwrap());
            (*items.add(i)).set_id(res["_id"].take().as_str().unwrap().to_owned());
        }
    }
}

#[must_use]
pub fn send_bulk<T: Serialize>(
    client: &mut Elasticsearch,
    parts: BulkParts,
    operations: Vec<BulkOperation<T>>,
) -> Option<BulkResult> {
    match wait4(client.bulk(parts).body(operations).send()) {
        Ok(resp) => {
            if !resp.status_code().is_success() {
                error!("Bulk failed: {}", resp.status_code());
            }
            wait4(resp.json::<BulkResult>())
                .map_err(|e| error!("Failed to parse Bulk response: {e}"))
                .ok()
        }
        Err(e) => {
            error!("Bulk failed: {e}");
            None
        }
    }
}

#[must_use]
pub fn send_search<'a, 'b, C: Body>(request: Search<'a, 'b, C>) -> Option<SearchResult> {
    match wait4(request.send()) {
        Ok(resp) => {
            if !resp.status_code().is_success() {
                error!("Search failed: {}", resp.status_code());
            }
            wait4(resp.json::<SearchResult>())
                .map_err(|x| error!("Failed to parse Search response: {x}"))
                .ok()
        }
        Err(e) => {
            error!("Failed to send request: {e}");
            None
        }
    }
}

/// Creates an index with explicity mappings.
/// If ElasticSearch responds, returns the response.
/// Else prints the error to log and returns `None`
fn create_index(
    client: &mut Elasticsearch,
    name: &str,
    mappings: Option<JsonValue>,
) -> Option<Response> {
    match wait4(
        client
            .indices()
            .create(indices::IndicesCreateParts::Index(name))
            .send(),
    ) {
        Ok(resp) => Some(resp),
        Err(err) => {
            error!(
                "Unable to create index: {}, status code {}",
                err.to_string(),
                err.status_code()
                    .map(|x| x.to_string())
                    .unwrap_or("<unavailable>".to_owned())
            );
            None
        }
    }
}

/// Checks if the index exists and creates it if not.
/// Returns `Ok` on success `Err` on failure.
/// Both cases are reported to logs.
fn check_create_index(
    client: &mut Elasticsearch,
    name: &str,
    mappings: Option<JsonValue>,
) -> Result<(), ()> {
    match wait4(
        client
            .indices()
            .get(indices::IndicesGetParts::Index(&[name]))
            .send(),
    ) {
        Ok(res) => {
            if !res.status_code().is_success() {
                warn!("Index {name} not found! Creating it anew");
                if let Some(r) = create_index(client, name, mappings) {
                    if let Err(e) = r.error_for_status_code() {
                        error!(
                            "Unable to create index {name}: {}, status code: {}",
                            e.to_string(),
                            e.status_code().unwrap()
                        );
                        Err(())
                    } else {
                        info!("Index {name} created");
                        Ok(())
                    }
                } else {
                    Err(())
                }
            } else {
                info!("Index {name} alredy exists");
                Ok(())
            }
        }
        Err(err) => {
            error!(
                "{},\nstatus code {}",
                err.to_string(),
                err.status_code()
                    .map(|x| x.as_str().to_owned())
                    .unwrap_or("<unavailable>".to_owned())
            );
            Err(())
        }
    }
}

/// Deletes the index. Returns `1` on success, `0` on failure.
pub fn delete_index(client: &mut Elasticsearch, name: &str) -> i32 {
    match wait4(
        client
            .indices()
            .delete(indices::IndicesDeleteParts::Index(&[name]))
            .send(),
    ) {
        Ok(resp) => {
            if resp.status_code().is_success() {
                info!("Index {name} deleted");
                1
            } else {
                error!(
                    "Error deleteting index {name}, status code: {}",
                    resp.status_code()
                );
                0
            }
        }
        Err(e) => {
            error!("Unable to delete index: {}", e);
            0
        }
    }
}

#[no_mangle]
unsafe extern "C" fn init_client() -> *mut Elasticsearch {
    let log_target = env_logger::Target::Stderr;
    env_logger::Builder::new()
        .target(log_target)
        .format_source_path(false)
        .format_timestamp(None)
        .filter_level(log::LevelFilter::Info)
        .init();

    let transport = TransportBuilder::new(SingleNodeConnectionPool::new(
        Url::parse("http:localhost:9200/").unwrap()
    ))
    // .auth(auth::Credentials::Basic("elastic".into(), "_u8p1dmbV4snhhRMO0NI".into()))
    // .request_body_compression(true)
    .build();
    let client = match transport {
        Ok(transport) => {
            let els: *mut Elasticsearch = std::alloc::alloc(Layout::new::<Elasticsearch>()).cast();
            if els.is_null() {
                return null_mut();
            }
            els.write(Elasticsearch::new(transport));
            els
        }
        Err(e) => {
            error!("Error creating transport: {}", e);
            return null_mut();
        }
    };

    if check_create_index(client.as_mut_unchecked(), TASK1_INDEX, None).is_err()
        || check_create_index(client.as_mut_unchecked(), TASK2_INDEX, None).is_err()
    {
        error!("Failed to initialize client! Shutting down");
        return null_mut();
    }

    client
}

#[no_mangle]
pub extern "C" fn close_client(handle: *mut Elasticsearch) {
    info!("Closing client. Goodbye");
    if !handle.is_null() {
        unsafe {
            drop(Box::from_raw(handle));
        }
    }
}
