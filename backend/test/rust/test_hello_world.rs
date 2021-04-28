mod test_hello_world_rust;
use test_hello_world_rust as test;

fn main() {
    unsafe {

    assert!(test::test::hello_world().to_str().unwrap() == "hello world");

    }
}
