use core::{slice, str};
use elasticsearch::*;
use log::{error, info, trace};
use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::ptr::null_mut;

use crate::{send_bulk, wait4, BufferString, ElasticId, TASK1_INDEX};

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

#[repr(C)]
pub struct RetrieveParts {
    pub ptr: *mut FactoryPart,
    pub count: isize,
}

/// Retrieves all parts from ElasticSearch.
///
/// If no parts are retrieved or an error occured, returns null pointer.
/// On success returns a valid pointer allocated by malloc
#[no_mangle]
pub extern "C" fn retrieve_all(handle: &mut Elasticsearch) -> *mut FactoryPart {
    let srch = handle
        .search(SearchParts::Index(&[TASK1_INDEX]))
        .pretty(true)
        .human(true)
        // .header(headers::ACCEPT_ENCODING)
        .to_owned()
        .send();
    match wait4(srch) {
        Ok(resp) => {
            info!("{:#?}", resp.headers());
            info!("{:?}", wait4(resp.text()));
        }
        Err(_) => todo!(),
    }
    // let allocator = std::alloc::System;
    // if let Ok(layout) = std::alloc::Layout::array::<FactoryPart>(10) {
    //     return allocator
    //         .allocate(layout)
    //         .ok()
    //         .map(|x| x.as_mut_ptr().cast())
    //         .unwrap_or(null_mut());
    // }
    null_mut()
}

// #[no_mangle]
// pub extern "C" fn
