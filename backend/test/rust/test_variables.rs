mod test_variables_rust;
use test_variables_rust as test;

fn main() {
    unsafe {

    {
        let e = test::TestSimpleEnum::Enum1 as u32;
        assert!(e == 1);

        let e = test::TestSimpleEnum::Enum2 as u32;
        assert!(e == 2);

        let e = test::TestSimpleEnum::Enum1;

        let e_str = match e {
          test::TestSimpleEnum::Enum1 => "Enum1",
          test::TestSimpleEnum::Enum2 => "Enum2",
        };

        assert!(e_str == "Enum1");
    }

    {
        let e = test::TestScopedEnum::ScopedEnum1 as u32;
        assert!(e == 1);

        let e = test::TestScopedEnum::ScopedEnum2 as u32;
        assert!(e == 2);
    }

    assert!(test::ANONYMOUS_ENUM_1 == 1);
    assert!(test::ANONYMOUS_ENUM_2 == 2);

    {
        let e = test::TestEnumInAnonymousNamespace::EnumInAnonymousNamespace1 as u32;
        assert!(e == 1);

        let e = test::TestEnumInAnonymousNamespace::EnumInAnonymousNamespace2 as u32;
        assert!(e == 2);
    }

    assert!(test::get_test_unsigned_constexpr_1() == 1);
    assert!(test::get_test_double_constexpr_1() == 1.0);

  // XXX macros

  }
}
