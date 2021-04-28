mod test_variables_rust;
use test_variables_rust::*;

fn main() {
    {
        let e = test::SimpleEnum::Enum1 as u32;
        assert!(e == 1);

        let e = test::SimpleEnum::Enum2 as u32;
        assert!(e == 2);

        let e = test::SimpleEnum::Enum1;

        let e_str = match e {
          test::SimpleEnum::Enum1 => "Enum1",
          test::SimpleEnum::Enum2 => "Enum2",
        };

        assert!(e_str == "Enum1");
    }

    {
        let e = test::ScopedEnum::ScopedEnum1 as u32;
        assert!(e == 1);

        let e = test::ScopedEnum::ScopedEnum2 as u32;
        assert!(e == 2);
    }

    assert!(test::ANONYMOUS_ENUM_1 == 1);
    assert!(test::ANONYMOUS_ENUM_2 == 2);

    {
        let e = test::EnumInAnonymousNamespace::EnumInAnonymousNamespace1 as u32;
        assert!(e == 1);

        let e = test::EnumInAnonymousNamespace::EnumInAnonymousNamespace2 as u32;
        assert!(e == 2);
    }

    unsafe {
        assert!(test::get_unsigned_constexpr() == 1);
        assert!(test::get_double_constexpr() == 1.0);
    }

    // XXX macros
}