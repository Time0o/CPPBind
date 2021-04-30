mod test_hello_world_rust;
use test_hello_world_rust::*;

fn main() {
    unsafe {

    assert!(test::hello_world().to_str().unwrap() == "hello world");

    }
}
