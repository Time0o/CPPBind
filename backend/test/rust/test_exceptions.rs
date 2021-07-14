use std::fmt;
use std::ffi::*;
use std::os::raw::*;

include!("rust_bind_error.rs");

include!("test_exceptions_rust.rs");

fn main() {
    unsafe {

    assert!(test_add_nothrow(1, 2) == 3);

    let err = match test_add_throw_runtime(1, 2) {
        Ok(_)  => String::from("no exception"),
        Err(e) => e.what,
    };

    assert!(err == "runtime error");

    let err = match test_add_throw_bogus(1, 2) {
        Ok(_)  => String::from("no exception"),
        Err(e) => e.what,
    };

    assert!(err == "exception");

    }
}
