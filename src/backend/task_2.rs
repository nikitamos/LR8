use std::{
    borrow::Borrow,
    ffi::c_char,
    fs::File,
    io::{Bytes, Write},
};

#[repr(C)]
pub enum Eeee {
    Nothing,
}

#[repr(C)]
pub struct Book {
    pub reg_num: u32,
    pub author: [c_char; 80],
    pub title: [c_char; 120],
    pub date: u32,
    pub house: [c_char; 80],
}

impl Book {
    pub fn bytes(&self) -> &[u8] {
        todo!()
    }
}
