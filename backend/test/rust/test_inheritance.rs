use std::ffi::*;

include!("test_inheritance_rust.rs");

fn main() {
    unsafe {

    {
        let derived = TestDerived::new();

        assert!(derived.func_1() == 1);
        assert!(derived.func_2() == 3);
        assert!(derived.func_abstract() == true);
    }

    {
        let base_1 = TestBase1::new();
        assert!(base_1.func_1() == 1);

        let base_2 = TestBase2::new();
        assert!(base_2.func_1() == 1);
        assert!(base_2.func_2() == 2);

        let base_protected = TestBaseProtected::new();
        assert!(base_protected.func_protected() == true);

        let base_private = TestBasePrivate::new();
        assert!(base_private.func_private() == true);
    }

    }
}
