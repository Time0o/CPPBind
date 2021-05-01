mod test_typedefs_rust;
use test_typedefs_rust::*;

fn main() {
    unsafe {

    {
        let b: BoolType = true;
        let not_b: BoolType = test::logical_not(b);
        assert!(not_b == false);
    }

    {
        let a: IntType1 = 1;
        let b: IntType2 = 2;
        let c: test::IntTypeCommon = test::add(a, b);
        assert!(c == 3);
    }

    {
        let s: test::ST = test::S::new();
        let s_ptr: test::SPtrT = &s as test::SPtrT;
        let _s_copy: test::ST = test::copy_s_ptr(s_ptr);
    }

    }
}
