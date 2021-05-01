mod test_overloaded_operators_rust;
use test_overloaded_operators_rust::*;

fn main() {
    unsafe {

    // XXX integer

    {
        let mut pointer = test::Pointer::new(1);

        let mut pointee1 = pointer.deref();
        assert!(pointee1.get_state() == 1);

        pointee1.set_state(2);
        let pointee2 = pointer.deref();
        assert!(pointee2.get_state() == 2);
    }

    {
        let class = test::ClassWithCallableMember::new();
        assert!(class.sum(1, 2) == 3);
    }

    }
}
