use std::ffi::*;
use std::os::raw::*;

include!("test_parameters_rust.rs");

fn main() {
    unsafe {

    // integer parameters
    assert!(test_add_signed_int(TEST_MIN_SIGNED_INT + 1, -1) == TEST_MIN_SIGNED_INT);
    assert!(test_add_signed_int(TEST_MAX_SIGNED_INT - 1, 1) == TEST_MAX_SIGNED_INT);
    assert!(test_add_unsigned_int(TEST_MAX_UNSIGNED_INT - 1, 1) == TEST_MAX_UNSIGNED_INT);
    assert!(test_add_large_signed_int(TEST_MIN_LARGE_SIGNED_INT + 1, -1) == TEST_MIN_LARGE_SIGNED_INT);
    assert!(test_add_large_signed_int(TEST_MAX_LARGE_SIGNED_INT - 1, 1) == TEST_MAX_LARGE_SIGNED_INT);

    // floating point parameters
    assert!((test_add_double(0.1, 0.2) - 0.3).abs() < test_get_epsilon_double());

    // boolean parameters
    assert!(test_not_bool(true) == false);
    assert!(test_not_bool(false) == true);

    // enum parameters
    assert!(matches!(test_not_bool_enum(TestBoolean::False), TestBoolean::True));
    assert!(matches!(test_not_bool_enum(TestBoolean::True), TestBoolean::False));

    assert!(matches!(test_not_bool_scoped_enum(TestBooleanScoped::False), TestBooleanScoped::True));
    assert!(matches!(test_not_bool_scoped_enum(TestBooleanScoped::True), TestBooleanScoped::False));

    // pointer parameters
    {
        let mut i1: i32 = 1;
        let i1_ptr: *mut i32 = &mut i1;

        let i2: i32 = 2;
        let i2_ptr: *const i32 = &i2;

        assert!(test_add_pointer(i1_ptr, i2_ptr) == i1_ptr && i1 == 3);
    }

    {
        let mut i1: i32 = 1;
        let mut i1_ptr: *mut i32 = &mut i1;
        let i1_ptr_ptr: *mut *mut i32 = &mut i1_ptr;

        let mut i2: i32 = 2;
        let mut i2_ptr: *mut i32 = &mut i2;
        let i2_ptr_ptr: *mut *mut i32 = &mut i2_ptr;

        assert!(test_add_pointer_to_pointer(i1_ptr_ptr, i2_ptr_ptr) == i1_ptr_ptr && i1 == 3);
    }

    // string parameters
    {
        let str1 = CString::new("abcd").unwrap();
        let str2 = CString::new("abdd").unwrap();

        assert!(test_min_string_parameters(&str1, &str2).to_str().unwrap() == "abcd");
    }

    // lvalue reference parameters
    {
        let mut i1: i32 = 1;
        let i2: i32 = 2;

        assert!(*test_add_lvalue_ref(&mut i1, &i2) == 3 && i1 == 3);
    }

    // rvalue reference parameters
    {
        let mut i1: i32 = 1;
        let i2: i32 = 2;

        assert!(test_add_rvalue_ref(&mut i1, &i2) == 3);
    }

    // no parameters
    assert!(test_one_no_parameters() == 1);

    // unused parameters
    assert!(test_add_unused_parameters(1, -1, 2, -1) == 3);

    }
}
