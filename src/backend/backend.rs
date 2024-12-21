#![feature(box_as_ptr)]
#![feature(allocator_api)]
#![feature(slice_ptr_get)]

use core::{slice, str};
use elasticsearch::*;
use http::{
    response::Response,
    transport::{SingleNodeConnectionPool, TransportBuilder},
    Url,
};
use log::{error, info, warn};
use serde::{de::Visitor, Deserialize, Serialize};
use serde_json::{json, Value};
use std::{
    ffi::{c_char, CStr, CString},
    future::Future,
    process::exit,
    ptr::{null, null_mut},
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

#[repr(transparent)]
pub struct BufferString(pub [c_char; 80]);

impl Serialize for BufferString {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        let s = unsafe { str::from_utf8(slice::from_raw_parts(self.0.as_ptr().cast(), 80)) }
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

/// Creates an index with explicity mappings.
/// If ElasticSearch responds, returns the response.
/// Else prints the error to log and returns `None`
fn create_index(
    client: &mut Elasticsearch,
    name: &str,
    mappings: Option<Value>,
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
    mappings: Option<Value>,
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

#[no_mangle]
pub extern "C" fn init_client() -> *mut Elasticsearch {
    let log_target = env_logger::Target::Stderr;
    env_logger::Builder::new()
        .target(log_target)
        .format_source_path(false)
        .format_timestamp(None)
        .filter_level(log::LevelFilter::Info)
        .init();

    let transport = TransportBuilder::new(SingleNodeConnectionPool::new(
        Url::from_str("http://localhost:9200").unwrap(),
    ))
    .build();
    let mut client = match transport {
        Ok(transport) => Box::new(Elasticsearch::new(transport)),
        Err(e) => {
            error!("Error creating ElasticSearch client: {}", e);
            return null_mut();
        }
    };

    if check_create_index(&mut client, TASK1_INDEX, None).is_err()
        || check_create_index(&mut client, TASK2_INDEX, None).is_err()
    {
        error!("Failed to initialize client! Shutting down");
        return null_mut();
    }

    exit(-1);
    Box::as_mut_ptr(&mut client)
}

#[no_mangle]
pub extern "C" fn close_client(handle: *mut Elasticsearch) {
    if !handle.is_null() {
        unsafe {
            drop(Box::from_raw(handle));
        }
    }
}
