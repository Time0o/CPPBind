mod test_nested_records_rust;
use test_nested_records_rust::*;

fn main() {
    unsafe {

    {
        let _obj = test::Toplevel::new();
    }

    {
        let obj = test::toplevel::NestedPublic::new();

        let t: test::toplevel::nested_public::T = 0;
        let e: test::toplevel::nested_public::E = test::toplevel::nested_public::E::V;
        test::Toplevel::func(&obj, t, e);
    }

    {
        let _obj = test::toplevel::nested_public::NestedNestedPublic::new();
    }

    }
}
