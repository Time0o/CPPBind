use std::ffi::*;

include!("test_templates_rust.rs");

fn main() {
    unsafe {

    // function templates
    assert!(test_add_int(1, 2) == 3);
    assert!(test_add_long(1, 2) == 3);

    // multiple template parameters
    assert!(test_add_int(1, 2) == 3);
    assert!(test_mult_int_long(2, 3) == 6);
    assert!(test_mult_long_int(2, 3) == 6);

    // non-type template parameters
    assert!(test_inc_int_1(1) == 2);
    assert!(test_inc_int_2(1) == 3);
    assert!(test_inc_int_3(1) == 4);

    // default template arguments
    assert!(test_pi_int() == 3);
    assert!(test_pi_double() == test_get_pi());

    // template parameter packs
    assert!(test_sum_int_int(1, 2) == 3);
    assert!(test_sum_int_int_long_long(1, 2, 3, 4) == 10);

    // record templates
    {
        let mut any_stack = TestAnyStackInt::new();

        any_stack.push_int(1);
        any_stack.push_double(3.14);

        assert!(any_stack.pop_double() == 3.14);
        assert!(any_stack.pop_int() == 1);

        assert!(any_stack.empty());
    }

    assert!(TestCalcInt::add(1, 2) == 3);
    assert!(TestCalcInt::sub(1, 2) == -1);

    }
}
