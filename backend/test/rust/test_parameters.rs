mod test_parameters_rust;
use test_parameters_rust::*;

fn main() {
    unsafe {

    // integer parameters
    assert!(test::add_signed_int(test::MIN_SIGNED_INT + 1, -1) == test::MIN_SIGNED_INT);
    assert!(test::add_signed_int(test::MAX_SIGNED_INT - 1, 1) == test::MAX_SIGNED_INT);
    assert!(test::add_unsigned_int(test::MAX_UNSIGNED_INT - 1, 1) == test::MAX_UNSIGNED_INT);
    assert!(test::add_large_signed_int(test::MIN_LARGE_SIGNED_INT + 1, -1) == test::MIN_LARGE_SIGNED_INT);
    assert!(test::add_large_signed_int(test::MAX_LARGE_SIGNED_INT - 1, 1) == test::MAX_LARGE_SIGNED_INT);

    // floating point parameters
    assert!((test::add_double(0.1, 0.2) - 0.3).abs() < test::get_epsilon_double());

    // boolean parameters
    assert!(test::not_bool(true) == false);
    assert!(test::not_bool(false) == true);

    // enum parameters
    assert!(matches!(test::not_bool_enum(test::Boolean::False), test::Boolean::True));
    assert!(matches!(test::not_bool_enum(test::Boolean::True), test::Boolean::False));

    assert!(matches!(test::not_bool_scoped_enum(test::BooleanScoped::False), test::BooleanScoped::True));
    assert!(matches!(test::not_bool_scoped_enum(test::BooleanScoped::True), test::BooleanScoped::False));

    // pointer parameters
    {
        let mut i1: i32 = 1;
        let i1_ptr: *mut i32 = &mut i1;

        let i2: i32 = 2;
        let i2_ptr: *const i32 = &i2;

        assert!(test::add_pointer(i1_ptr, i2_ptr) == i1_ptr && i1 == 3);
    }

    {
        let mut i1: i32 = 1;
        let mut i1_ptr: *mut i32 = &mut i1;
        let i1_ptr_ptr: *mut *mut i32 = &mut i1_ptr;

        let mut i2: i32 = 2;
        let mut i2_ptr: *mut i32 = &mut i2;
        let i2_ptr_ptr: *mut *mut i32 = &mut i2_ptr;

        assert!(test::add_pointer_to_pointer(i1_ptr_ptr, i2_ptr_ptr) == i1_ptr_ptr && i1 == 3);
    }

    // string parameters
    {
        use std::ffi::CString;

        let str1 = CString::new("abcd").unwrap();
        let str2 = CString::new("abdd").unwrap();

        assert!(test::min_string_parameters(&str1, &str2).to_str().unwrap() == "abcd");
    }

    // lvalue reference parameters
    {
        let mut i1: i32 = 1;
        let i2: i32 = 2;

        assert!(*test::add_lvalue_ref(&mut i1, &i2) == 3 && i1 == 3);
    }

    // rvalue reference parameters
    {
        let mut i1: i32 = 1;
        let i2: i32 = 2;

        assert!(test::add_rvalue_ref(&mut i1, &i2) == 3);
    }

    // no parameters
    assert!(test::one_no_parameters() == 1);

    // unused parameters
    assert!(test::add_unused_parameters(1, -1, 2, -1) == 3);

    }
}
