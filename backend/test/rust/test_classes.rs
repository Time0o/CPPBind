use std::ffi::*;
use std::os::raw::*;

include!("test_classes_rust.rs");

fn main() {
    unsafe {

    // construction and deletion
    {
        let _trivial = TestTrivial::new();
    }

    // member access
    {
        let a_class = TestAClass::new_1();

        assert!(a_class.get_state() == 0);
    }

    assert!(TestAClass::get_num_destroyed() == 1);

    {
        let mut a_class = TestAClass::new_2(1);

        assert!(a_class.get_state() == 1);

        a_class.set_state(2);
        assert!(a_class.get_state() == 2);
    }

    assert!(TestAClass::get_num_destroyed() == 2);

    TestAClass::set_static_state(1);
    assert!(TestAClass::get_static_state() == 1);


    // copying and moving
    {
        let copyable_class = TestCopyableClass::new(1);
        assert!(copyable_class.get_state() == 1);

        assert!(TestCopyableClass::get_num_copied() == 0);

        let mut copyable_class_copy = copyable_class.clone();
        assert!(copyable_class_copy.get_state() == 1);

        assert!(TestCopyableClass::get_num_copied() == 1);

        copyable_class_copy.set_state(2);
        assert!(copyable_class.get_state() == 1);
        assert!(copyable_class_copy.get_state() == 2);

        let mut copyable_class_copy_assigned = TestCopyableClass::new(0);

        copyable_class_copy_assigned.clone_from(&copyable_class);
        assert!(copyable_class_copy_assigned.get_state() == 1);

        assert!(TestCopyableClass::get_num_copied() == 2);

        copyable_class_copy_assigned.set_state(2);
        assert!(copyable_class.get_state() == 1);
        assert!(copyable_class_copy_assigned.get_state() == 2);
    }

    // XXX moving

    // class parameters

    // value parameter
    {
        let mut a = TestClassParameter::new(1);
        let b = TestClassParameter::new(2);

        TestClassParameter::set_copyable(true);

        let c = test_add_class(&mut a, &b);
        assert!(a.get_state() == 1);
        assert!(c.get_state() == 3);

        TestClassParameter::set_copyable(false);
    }

    // pointer parameters
    {
        let mut a = TestClassParameter::new(1);
        let b = TestClassParameter::new(2);

        let c = test_add_class_pointer(&mut a, &b);
        assert!(a.get_state() == 3);
        assert!(c.get_state() == 3);
    }

    // lvalue reference parameters
    {
        let mut a = TestClassParameter::new(1);
        let b = TestClassParameter::new(2);

        let c = test_add_class_lvalue_ref(&mut a, &b);
        assert!(a.get_state() == 3);
        assert!(c.get_state() == 3);
    }

    // rvalue reference parameters
    {
        let mut a = TestClassParameter::new(1);

        TestClassParameter::set_moveable(true);

        test_noop_class_rvalue_ref(&mut a);

        assert!(a.was_moved());

        TestClassParameter::set_moveable(false);
    }

    }
}
