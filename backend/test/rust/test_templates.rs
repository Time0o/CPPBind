mod test_templates_rust;
use test_templates_rust::*;

fn main() {
    unsafe {

    // function templates
    assert!(test::add_int(1, 2) == 3);
    assert!(test::add_long(1, 2) == 3);

    // multiple template parameters
    assert!(test::add_int(1, 2) == 3);
    assert!(test::mult_int_long(2, 3) == 6);
    assert!(test::mult_long_int(2, 3) == 6);

    // non-type template parameters
    assert!(test::inc_int_1(1) == 2);
    assert!(test::inc_int_2(1) == 3);
    assert!(test::inc_int_3(1) == 4);

    // default template arguments
    assert!(test::pi_int() == 3);
    assert!(test::pi_double() == test::get_pi());

    // template parameter packs
    assert!(test::sum_int_int(1, 2) == 3);
    assert!(test::sum_int_int_long_long(1, 2, 3, 4) == 10);

    // record templates
    {
        let mut any_stack = test::AnyStackInt::new();

        any_stack.push_int(1);
        any_stack.push_double(3.14);

        assert!(any_stack.pop_double() == 3.14);
        assert!(any_stack.pop_int() == 1);

        assert!(any_stack.empty());
    }

    }
}
