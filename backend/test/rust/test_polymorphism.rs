mod test_polymorphism_rust;
use test_polymorphism_rust::*;
use test_polymorphism_rust::test::Base1Trait;
use test_polymorphism_rust::test::Base2Trait;

fn main() {
    unsafe {

    let derived_1 = test::Derived1::new();
    let derived_2 = test::Derived2::new();

    {
      let mut v: Vec<&dyn Base1Trait> = vec![];
      v.push(&derived_1);
      v.push(&derived_2);

      assert!(test::func_1(v[0]) == 1);
      assert!(test::func_1(v[1]) == 2);
    }

    {
      let mut v: Vec<&dyn Base2Trait> = vec![];
      v.push(&derived_1);
      v.push(&derived_2);

      assert!(test::func_2(v[0]) == -1);
      assert!(test::func_2(v[1]) == -2);
    }

    }
}
