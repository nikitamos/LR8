use core::{slice, str};
use elasticsearch::*;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::{
    alloc::Allocator,
    ptr::null_mut,
};

use crate::BufferString;

#[repr(C, i32)]
#[derive(Serialize, Deserialize)]
pub enum Material {
    /// Марка
    Steel(i32) = 0,
    /// Проценстное содержание олова
    Brass(f32) = 1,
}

#[repr(C)]
#[derive(Serialize)]
pub struct FactoryPart {
    pub name: BufferString,
    pub count: i32,
    pub department_no: i32,
    pub material: Material,
    pub weight: f32,
    pub volume: f32,
}

#[no_mangle]
pub extern "C" fn add_parts(handle: &mut Elasticsearch, parts: *const FactoryPart, count: i32) {
    let operations: Vec<BulkOperation<Value>> =
        unsafe { slice::from_raw_parts(parts, count as usize) }
            .iter()
            .map(|x| BulkOperation::create(serde_json::to_value(x).unwrap()).into())
            .collect();
    let response = handle
        .bulk(BulkParts::Index(crate::TASK1_INDEX))
        .body(operations)
        .send();
    let rt = tokio::runtime::Runtime::new().expect("Unable to create a tokio runtime");
    let response = rt.block_on(response);
    match response {
        Ok(res) => todo!(),
        Err(err) => todo!(),
    }
}

#[no_mangle]
pub extern "C" fn retrieve_all(handle: &mut Elasticsearch) -> *mut FactoryPart {
    let allocator = std::alloc::System;
    if let Ok(layout) = std::alloc::Layout::array::<FactoryPart>(10) {
        return allocator
            .allocate(layout)
            .ok().map(|x| x.as_mut_ptr().cast())
            .unwrap_or(null_mut());
    }
    null_mut()
}

// #[no_mangle]
// pub extern "C" fn
