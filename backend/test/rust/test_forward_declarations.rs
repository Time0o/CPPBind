mod test_forward_declarations_rust;
use test_forward_declarations_rust::*;

fn main() {
    unsafe {

    assert!(test::add_1(1, 2) == 3);
    assert!(test::add_2(1, 2, 3) == 6);

    {
        let adder = test::Adder::new();

        assert!(adder.add(1, 2) == 3);
    }

    }
}
