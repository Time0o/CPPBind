mod test_hello_world_rust;
use test_hello_world_rust as test;

fn main() {
    unsafe {

    assert!(test::test_hello_world() == "hello world");

    }
}
