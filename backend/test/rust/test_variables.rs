use std::ffi::*;

include!("test_variables_rust.rs");

fn main() {
    unsafe {
        assert!(test_get_unsigned_constexpr() == 1);
        assert!(test_get_double_constexpr() == 1.0);

        assert!(test_get_int_var() == 1);

        test_set_int_var(2);
        assert!(test_get_int_var() == 2);

        let int_ref = test_get_int_ref();
        assert!(*int_ref == 2);

        *int_ref = 3;
        assert!(*test_get_int_ref() == 3);
        assert!(*test_get_int_const_ref() == 3);
    }
}
