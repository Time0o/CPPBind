use std::os::raw::*;

include!("test_variables_rust.rs");

fn main() {
    {
        let e = TestSimpleEnum::Enum1 as u32;
        assert!(e == 1);

        let e = TestSimpleEnum::Enum2 as u32;
        assert!(e == 2);

        let e = TestSimpleEnum::Enum1;

        let e_str = match e {
          TestSimpleEnum::Enum1 => "Enum1",
          TestSimpleEnum::Enum2 => "Enum2",
        };

        assert!(e_str == "Enum1");
    }

    {
        let e = TestScopedEnum::ScopedEnum1 as u32;
        assert!(e == 1);

        let e = TestScopedEnum::ScopedEnum2 as u32;
        assert!(e == 2);
    }

    assert!(TEST_ANONYMOUS_ENUM_1 == 1);
    assert!(TEST_ANONYMOUS_ENUM_2 == 2);

    {
        let e = TestEnumInAnonymousNamespace::EnumInAnonymousNamespace1 as u32;
        assert!(e == 1);

        let e = TestEnumInAnonymousNamespace::EnumInAnonymousNamespace2 as u32;
        assert!(e == 2);
    }

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

    // XXX macros
}
