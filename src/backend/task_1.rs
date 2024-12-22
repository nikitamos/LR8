use core::{slice, str};
use elasticsearch::*;
use log::{error, info, trace};
use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::alloc::{self, Allocator};
use std::ptr::null_mut;

use crate::{send_bulk, send_search, BufferString, ElasticId, TASK1_INDEX};

#[repr(C, i32)]
#[derive(Serialize, Deserialize, Clone, Debug)]
pub enum Material {
    /// Марка
    Steel(i32) = 0,
    /// Процентное содержание олова
    Brass(f32) = 1,
}

#[repr(C)]
#[derive(Deserialize, Serialize, Clone, Debug)]
pub struct FactoryPart {
    pub name: BufferString,
    pub count: i32,
    pub department_no: i32,
    pub material: Material,
    pub weight: f32,
    pub volume: f32,
    #[serde(skip_serializing_if = "Option::is_none")]
    _id: Option<Box<String>>,
}

impl ElasticId for FactoryPart {
    fn get_id(&self) -> Option<&str> {
        self._id.as_deref().map(|x| x.as_str())
    }

    fn set_id(&mut self, id: String) {
        self._id = Some(Box::new(id))
    }
}

/// Returns the `count` on success, zero on failure
#[no_mangle]
pub extern "C" fn add_parts(
    handle: &mut Elasticsearch,
    parts: *mut FactoryPart,
    count: i32,
) -> i32 {
    info!("Adding {count} parts");
    if count == 0 {
        return 0;
    }
    unsafe {
        trace!(
            "First part to add: {}",
            serde_json::to_string_pretty(&(*parts)).unwrap()
        )
    };
    let parts = unsafe { slice::from_raw_parts_mut(parts, count as usize) };
    let operations: Vec<BulkOperation<JsonValue>> =
        parts
            .iter()
            .map(|x| BulkOperation::create(serde_json::to_value(x.clone()).unwrap()).into())
            .collect();
    if let Some(res) = send_bulk(handle, BulkParts::Index(TASK1_INDEX), operations) {
        if res.is_ok() {
            info!("Succeed");
            res.write_ids(parts);
            count
        } else {
            error!("Request error; I don't know what to do.");
            0
        }
    } else {
        0
    }
}

/// Retrieves all parts from ElasticSearch.
///
/// If no parts are retrieved or an error occured, returns null pointer.
/// On success returns a valid pointer allocated by malloc
#[no_mangle]
pub extern "C" fn retrieve_all(
    handle: &mut Elasticsearch,
    parts: &mut *mut FactoryPart,
    size: &mut i32,
) {
    *parts = null_mut();
    *size = -1;
    info!("Retrieving parts from an existing index");
    match send_search(handle.search(SearchParts::Index(&[TASK1_INDEX]))) {
        Some(result) => {
            info!("Search succeed, {} documents in the index", result.count());
            let allocator = alloc::System;
            if (result.count()) == 0 {
                *size = 0;
                return;
            }
            if let Ok(layout) = alloc::Layout::array::<FactoryPart>(result.count()) {
                if let Ok(ptr) = allocator.allocate(layout) {
                    *parts = ptr.as_mut_ptr().cast();
                    *size = result.count() as i32;
                    unsafe {result.populate_array(*parts, *size)};
                    // info!("Populated")
                } else {
                    error!("Failed to allocate memory");
                }
            } else {
                error!("Failed to allocate memory");
            }
        },
        None => error!("Retrieving parts failed")
    }
}

// #[no_mangle]
// pub extern "C" fn
