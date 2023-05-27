use std::ffi::*;

include!("test_typedefs_rust.rs");

fn main() {
    unsafe {

    {
      let b: BoolType = true;
      let not_b: BoolType = test_logical_not(b);
      assert_eq!(not_b, false);
    }

    {
      let a: IntType1 = 1;
      let b: Scope1Scope2IntType2 = 2;
      let c: TestIntTypeCommon = test_add(a, b);
      assert_eq!(c, 3);
    }

    {
      let s: TestST = TestS::new();
      let s_ptr: TestSPtrT = &s;
      let _s_copy: TestST = test_copy_s_ptr(s_ptr);
    }

    }
}
