mod test_nested_records_rust;
use test_nested_records_rust::*;

fn main() {
    unsafe {

    {
        let _obj = test::Toplevel::new();
    }

    {
        let obj = test::ToplevelNestedPublic::new();

        let t: test::ToplevelNestedPublicT = 0;
        let e: test::ToplevelNestedPublicE = test::ToplevelNestedPublicE::V;
        test::Toplevel::func(&obj, t, e);
    }

    {
        let _obj = test::ToplevelNestedPublicNestedNestedPublic::new();
    }

    }
}
