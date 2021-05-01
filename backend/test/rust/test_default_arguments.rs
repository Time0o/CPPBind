mod test_default_arguments_rust;
use test_default_arguments_rust::*;

fn main() {
    unsafe {

    assert!(test::pow_default_arguments(test::get_e(), 2, true, test::Round::Upward) == 8.0);

    assert!(test::default_large_signed(test::LARGE_SIGNED) == test::LARGE_SIGNED);
    assert!(test::default_constexpr_function_call(test::func_constexpr()) == test::func_constexpr());
    assert!(test::default_function_call(test::func()) == test::func());

    let mut a: i32 = 0;
    test::default_ref(&mut a, 42);
    assert!(a == 42);

    {
        let mut a_class = test::AClass::new(42);
        let a_class_copy = test::default_record(&mut a_class);
        assert!(a_class_copy.get_value() == 42);
    }

    assert!(test::default_template_unsigned_int(1) == 1);
    assert!(test::default_template_after_parameter_pack_unsigned_int_int_int(1, 2, 3) == 3);

    }
}
