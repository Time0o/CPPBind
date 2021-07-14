use std::ffi::*;
use std::os::raw::*;

include!("test_default_arguments_rust.rs");

fn main() {
    unsafe {

    assert!(test_pow_default_arguments(test_get_e(), 2, true, TestRound::Upward) == 8.0);

    assert!(test_default_large_signed(TEST_LARGE_SIGNED) == TEST_LARGE_SIGNED);
    assert!(test_default_constexpr_function_call(test_func_constexpr()) == test_func_constexpr());
    assert!(test_default_function_call(test_func()) == test_func());

    let mut a: i32 = 0;
    test_default_ref(&mut a, 42);
    assert!(a == 42);

    {
        let mut a_class = TestAClass::new(42);
        let a_class_copy = test_default_record(&mut a_class);
        assert!(a_class_copy.get_value() == 42);
    }

    assert!(test_default_template_unsigned_int(1) == 1);
    assert!(test_default_template_after_parameter_pack_unsigned_int_int_int(1, 2, 3) == 3);

    }
}
