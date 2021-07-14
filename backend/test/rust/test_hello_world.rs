use std::ffi::*;
use std::os::raw::*;

include!("test_hello_world_rust.rs");

fn main() {
    unsafe {

    assert!(test_hello_world().to_str().unwrap() == "hello world");

    }
}
