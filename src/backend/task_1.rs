use core::{slice, str};
use elasticsearch::*;
use log::{error, info, trace};
use serde::{Deserialize, Serialize};
use serde_json::{json, Value as JsonValue};
use std::alloc::{self, Allocator};
use std::ptr::{null_mut, NonNull};

use crate::{create_index, delete_index, send_bulk, send_delete, send_search, BufferString, ElasticId, TASK1_INDEX};

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

    fn drop_id(&mut self) {
        drop(self._id.take())
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
    let operations: Vec<BulkOperation<JsonValue>> = parts
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
                    unsafe { result.populate_array(*parts, *size) };
                    // info!("Populated")
                } else {
                    error!("Failed to allocate memory");
                }
            } else {
                error!("Failed to allocate memory");
            }
        }
        None => error!("Retrieving parts failed"),
    }
}

/// Deletes all parts in the array.
/// It call an inner destructor and frees the memory
/// The `array` pointer is set to null, `size` is set to zero
#[no_mangle]
unsafe extern "C" fn free_all_parts(array: &mut *mut FactoryPart, count: &mut i32) {
    if *count == 0 || array.is_null() {
        return;
    }
    let allocator = alloc::System;
    let layout = alloc::Layout::array::<FactoryPart>(*count as usize)
        .expect("Failed to deallocate an array!");
    let sliced = slice::from_raw_parts_mut(*array, *count as usize);
    for part in sliced {
        drop(part._id.take())
    }
    allocator.deallocate(NonNull::new_unchecked(array.cast()), layout);
    *array = null_mut();
    *count = 0;
}

/// Returns `1` on success, `0` on error
#[no_mangle]
unsafe extern "C" fn delete_all_from_index(
    handle: &mut Elasticsearch,
    array: &mut *mut FactoryPart,
    count: &mut i32,
) -> i32 {
    free_all_parts(array, count);

    if delete_index(handle, TASK1_INDEX) != 1 {
        panic!("Error deleting index");
    }
    match create_index(handle, TASK1_INDEX, None) {
        Some(resp) => {
            if !resp.status_code().is_success() {
                error!("Index deleted, but could not be recreated");
                0
            } else {
                info!("Index successfully deleted and creted anew");
                1
            }
        }
        None => {
            error!("Index deleted, but could not be recreated!");
            0
        }
    }
}

#[repr(C)]
pub struct PartSearchResult {
    pub res: *mut FactoryPart,
    pub count: i32,
}

fn get_part_field(part: &mut FactoryPart, field: i32) -> JsonValue {
    match field {
        0 => json!({"name": part.name}),
        1 => json!({"department_no": part.department_no}),
        2 => json!({"weight": part.weight}),
        3 => json!({"material": part.material}),
        4 => json!({"volume": part.volume}),
        5 => json!({"count": part.count}),
        _ => panic!("Wrong Field!"),
    }
}

#[no_mangle]
pub extern "C" fn search_for_part(
    handle: &mut Elasticsearch,
    part: &mut FactoryPart,
    field: i32,
) -> PartSearchResult {
    let body = json!({
        "query": {
            "match": get_part_field(part, field)
        }
    });
    let req = handle.search(SearchParts::Index(&[TASK1_INDEX])).body(body);
    match send_search(req) {
        Some(res) => {
            let count = res.count();
            info!("Search succeeded, found {} documents", res.count());
            let allocator = alloc::System;
            let layout =
                alloc::Layout::array::<FactoryPart>(count).expect("Failed to allocate memory");
            if let Ok(ptr) = allocator.allocate(layout) {
                let ptr: *mut FactoryPart = ptr.as_mut_ptr().cast();
                unsafe { res.populate_array(ptr, count as i32) };
                PartSearchResult {
                    res: ptr,
                    count: count as i32,
                }
            } else {
                error!("Failed to allocate memory!");
                PartSearchResult {
                    res: null_mut(),
                    count: 0,
                }
            }
        }
        None => {
            error!("Search failed!");
            PartSearchResult {
                res: null_mut(),
                count: 0,
            }
        }
    }
}

/// Deletes the document from the index and calls the inner destructor
/// The memory should be cleared manually after caling this function
#[no_mangle]
pub extern "C" fn delete_factory_document(
    handle: &mut Elasticsearch,
    doc: &mut FactoryPart,
) -> i32 {
    match send_delete(handle.delete(DeleteParts::IndexId(TASK1_INDEX, doc.get_id().unwrap()))) {
        Some(_) => {
            info!("Document {} deleted", doc.get_id().unwrap());
            doc.drop_id();
            1
        }
        None => {
            error!("Failed to delete the document {}", doc.get_id().unwrap());
            0
        }
    }
}
