mod test_overloaded_operators_rust;
use test_overloaded_operators_rust::*;

fn main() {
    unsafe {

    {
        let mut i1 = test::Integer::new(2);
        assert!(i1.deref() == 2);

        let mut i2 = test::Integer::new(3);
        assert!(i2.deref() == 3);

        let i3 = test::mult(&i1, &i2);
        assert!(i3.deref() == 6);

        let i1_inc = test::Integer::inc_pre(&mut i1);
        assert!(i1_inc.deref() == 3);
        assert!(i1.deref() == 3);

        let i2_inc = test::Integer::inc_post(&mut i2);
        assert!(i2_inc.deref() == 3);
        assert!(i2.deref() == 4);

        let i0 = test::Integer::new(0);
        assert!(!i0.cast_bool());
        assert!(i1.cast_bool());
    }

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
