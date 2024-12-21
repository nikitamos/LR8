#![feature(box_as_ptr)]
#![feature(allocator_api)]
#![feature(slice_ptr_get)]
#![feature(mem_copy_fn)]

use core::{slice, str};
use elasticsearch::*;
use http::{
    transport::{SingleNodeConnectionPool, TransportBuilder},
    Url,
};
use serde::{de::Visitor, Deserialize, Serialize};
use serde_json::{json, Value};
use std::{
    alloc::Allocator,
    ffi::{c_char, CStr, CString},
    ptr::{null, null_mut},
    str::FromStr,
};

// #[no_mangle]
//extern "C" const STEEL_DENSITY: f32 = 8000f32;

pub const TASK1_INDEX: &str = "task1_factory";

pub mod task_1;

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
impl<'de> Visitor<'de> for BufStrVisitor {
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

#[no_mangle]
pub extern "C" fn init_client() -> *mut Elasticsearch {
    let transport = TransportBuilder::new(SingleNodeConnectionPool::new(
        Url::from_str("localhost:9200").unwrap(),
    ))
    .build();
    match transport {
        Ok(transport) => {
            let client = Box::new(Elasticsearch::new(transport));
            Box::into_raw(client)
        }
        Err(e) => {
            eprintln!("Error creating ElasticSearch client: {}", e.to_string());
            null_mut()
        }
    }
}

#[no_mangle]
pub extern "C" fn create_document(
    handle: Option<&mut Elasticsearch>,
    name: *const c_char,
) -> *const u8 {
    if let None = handle {
        return null();
    }
    let els = unsafe { handle.unwrap_unchecked() };
    let res = unsafe { CStr::from_ptr(name) }.to_str();
    match res {
        Ok(name) => {
            let parts = CreateParts::IndexId(name, "");
            let x = els.create(parts);
            //     x.body(json!(r#"{
            //     "mappings": {

            //     }
            // }"#));
            dbg!(x);
            null()
        }
        Err(e) => {
            eprintln!("Ошибка декодирования utf-8: {}", e);
            null()
        }
    }
}

#[no_mangle]
pub extern "C" fn close_client(handle: *mut Elasticsearch) -> () {
    if !handle.is_null() {
        unsafe {
            drop(Box::from_raw(handle));
        }
    }
}
