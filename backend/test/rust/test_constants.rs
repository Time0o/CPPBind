use std::os::raw::*;

include!("test_constants_rust.rs");

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

    assert!(TEST_DUPLICATE_ENUM_1 == 1);
    assert!(TEST_DUPLICATE_ENUM_2 == 1);

    {
        let e = TestEnumInAnonymousNamespace::EnumInAnonymousNamespace1 as u32;
        assert!(e == 1);

        let e = TestEnumInAnonymousNamespace::EnumInAnonymousNamespace2 as u32;
        assert!(e == 2);
    }

    // XXX macros
}
