use std::ffi::*;
use std::os::raw::*;

include!("test_overloaded_operators_rust.rs");

fn main() {
    unsafe {

    {
        let mut i1 = TestInteger::new(2);
        assert!(i1.deref() == 2);

        let mut i2 = TestInteger::new(3);
        assert!(i2.deref() == 3);

        let i3 = test_mult(&i1, &i2);
        assert!(i3.deref() == 6);

        let i1_inc = TestInteger::inc_pre(&mut i1);
        assert!(i1_inc.deref() == 3);
        assert!(i1.deref() == 3);

        let i2_inc = TestInteger::inc_post(&mut i2);
        assert!(i2_inc.deref() == 3);
        assert!(i2.deref() == 4);

        let i0 = TestInteger::new(0);
        assert!(!i0.cast_bool());
        assert!(i1.cast_bool());
    }

    {
        let mut pointer = TestPointer::new(1);

        let mut pointee1 = pointer.deref();
        assert!(pointee1.get_state() == 1);

        pointee1.set_state(2);
        let pointee2 = pointer.deref();
        assert!(pointee2.get_state() == 2);
    }

    {
        let class = TestClassWithCallableMember::new();
        assert!(class.sum(1, 2) == 3);
    }

    }
}
