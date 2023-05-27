use std::ffi::*;

include!("test_polymorphism_rust.rs");

fn main() {
    unsafe {

    let derived_1 = TestDerived1::new();
    let derived_2 = TestDerived2::new();

    {
      let v: Vec<&dyn TestBase1Trait> = vec![&derived_1, &derived_2];

      assert!(test_func_1(v[0]) == 1);
      assert!(test_func_1(v[1]) == 2);
    }

    {
      let v: Vec<&dyn TestBase2Trait> = vec![&derived_1, &derived_2];

      assert!(test_func_2(v[0]) == -1);
      assert!(test_func_2(v[1]) == -2);
    }

    }
}
