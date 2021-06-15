mod test_inheritance_rust;
use test_inheritance_rust::*;
use test_inheritance_rust::test::Base1Trait as _;
use test_inheritance_rust::test::Base2Trait as _;
use test_inheritance_rust::test::BaseAbstractTrait as _;

fn main() {
    unsafe {

    {
        let derived = test::Derived::new();

        assert!(derived.func_1() == 1);
        assert!(derived.func_2() == 3);
        assert!(derived.func_abstract() == true);
    }

    {
        let base_1 = test::Base1::new();
        assert!(base_1.func_1() == 1);

        let base_2 = test::Base2::new();
        assert!(base_2.func_1() == 1);
        assert!(base_2.func_2() == 2);

        let base_protected = test::BaseProtected::new();
        assert!(base_protected.func_protected() == true);

        let base_private = test::BasePrivate::new();
        assert!(base_private.func_private() == true);
    }

    }
}
