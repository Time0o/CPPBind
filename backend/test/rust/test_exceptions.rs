mod test_exceptions_rust;
use test_exceptions_rust::*;

fn main() {
    unsafe {

    assert!(test::add_nothrow(1, 2) == 3);

    let err = match test::add_throw_runtime(1, 2) {
        Ok(_)  => String::from("no exception"),
        Err(e) => e.what,
    };

    assert!(err == "runtime error");

    let err = match test::add_throw_bogus(1, 2) {
        Ok(_)  => String::from("no exception"),
        Err(e) => e.what,
    };

    assert!(err == "exception");

    }
}
