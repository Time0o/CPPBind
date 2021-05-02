mod test_classes_rust;
use test_classes_rust::*;

fn main() {
    unsafe {

    // construction and deletion
    {
        let _trivial = test::Trivial::new();
    }

    // member access
    {
        let a_class = test::AClass::new_1();

        assert!(a_class.get_state() == 0);
    }

    assert!(test::AClass::get_num_destroyed() == 1);

    {
        let mut a_class = test::AClass::new_2(1);

        assert!(a_class.get_state() == 1);

        a_class.set_state(2);
        assert!(a_class.get_state() == 2);
    }

    assert!(test::AClass::get_num_destroyed() == 2);

    test::AClass::set_static_state(1);
    assert!(test::AClass::get_static_state() == 1);


    // XXX copying and moving
    {
        //let copyable_class = test::CopyableClass::new(1);
        //assert!(copyable_class.get_state() == 1);

        //assert!(test::CopyableClass::get_num_copied() == 0);

        //let mut copyable_class_copy = copyable_class.clone();
        //assert!(copyable_class_copy.get_state() == 1);

        //assert!(test::CopyableClass::get_num_copied() == 1);

        //copyable_class_copy.set_state(2);
        //assert!(copyable_class.get_state() == 1);
        //assert!(copyable_class_copy.get_state() == 2);

        //let mut copyable_class_copy_assigned = test::CopyableClass::new(0);

        //copyable_class_copy_assigned.clone_from(&copyable_class);
        //assert!(copyable_class_copy_assigned.get_state() == 1);

        //assert!(test::CopyableClass::get_num_copied() == 2);

        //copyable_class_copy_assigned.set_state(2);
        //assert!(copyable_class.get_state() == 1);
        //assert!(copyable_class_copy_assigned.get_state() == 2);
    }

    // XXX moving

    // class parameters

    // value parameter
    {
        let mut a = test::ClassParameter::new(1);
        let b = test::ClassParameter::new(2);

        test::ClassParameter::set_copyable(true);

        let c = test::add_class(&mut a, &b);
        assert!(a.get_state() == 1);
        assert!(c.get_state() == 3);

        test::ClassParameter::set_copyable(false);
    }

    // pointer parameters
    {
        let mut a = test::ClassParameter::new(1);
        let b = test::ClassParameter::new(2);

        let c = test::add_class_pointer(&mut a, &b);
        assert!(a.get_state() == 3);
        assert!(c.get_state() == 3);
    }

    // lvalue reference parameters
    {
        let mut a = test::ClassParameter::new(1);
        let b = test::ClassParameter::new(2);

        let c = test::add_class_lvalue_ref(&mut a, &b);
        assert!(a.get_state() == 3);
        assert!(c.get_state() == 3);
    }

    // rvalue reference parameters
    {
        let mut a = test::ClassParameter::new(1);

        test::ClassParameter::set_moveable(true);

        test::noop_class_rvalue_ref(&mut a);

        assert!(a.was_moved());

        test::ClassParameter::set_moveable(false);
    }

    }
}
