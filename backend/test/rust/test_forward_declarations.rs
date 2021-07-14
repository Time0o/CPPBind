use std::ffi::*;
use std::os::raw::*;

include!("test_forward_declarations_rust.rs");

fn main() {
    unsafe {

    assert!(test_add_1(1, 2) == 3);
    assert!(test_add_2(1, 2, 3) == 6);

    {
        let adder = TestAdder::new();

        assert!(adder.add(1, 2) == 3);
    }

    }
}
