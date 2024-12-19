#![feature(box_as_ptr)]

use auth::Credentials;
use core::{slice, str};
use elasticsearch::*;
use http::{
    transport::{SingleNodeConnectionPool, TransportBuilder},
    Url,
};
use std::{error::Error, ffi::{c_char, c_int, c_void, CStr, CString}, ptr::{null, null_mut, NonNull}, str::FromStr};

// #[no_mangle]
extern "C" const STEEL_DENSITY: f32 = 8000f32;


#[repr(C, i32)]
pub enum Material {
    /// Марка
    Steel(i32) = 0,
    /// Проценстное содержание олова
    Brass(f32) = 1,
}

#[repr(C)]
pub struct FactoryPart {
    pub name: [c_char; 80],
    pub count: i32,
    pub department_no: i32,
    pub material: Material,
    pub weight: f32,
    pub volume: f32,
}

#[no_mangle]
pub extern "C" fn init_client() -> *mut Elasticsearch {
    // let cred = Credentials::Basic("elastic".to_owned(), "".to_owned());
    // // let cp =  ConnectionPool::
    let transport = TransportBuilder::new(SingleNodeConnectionPool::new(
        Url::from_str("localhost:9200").unwrap(),
    ))
    // .auth(cred)
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
pub extern "C" fn add_item(item: FactoryPart) {
    todo!()
}

#[no_mangle]
pub extern "C" fn create_document(handle: Option<&mut Elasticsearch>, name: *const c_char) -> *const u8 {
  if let None = handle {
    return null();
  }
  let els = unsafe {handle.unwrap_unchecked()};
  let res = unsafe { CStr::from_ptr(name) }.to_str();
  match res {
    Ok(name) => {
      let parts = CreateParts::IndexId(name, "");
      let x = els.create(parts);
      dbg!(x);
      null()
    },
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
