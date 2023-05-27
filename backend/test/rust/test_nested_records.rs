use std::ffi::*;

include!("test_nested_records_rust.rs");

fn main() {
    unsafe {

    {
        let _obj = TestToplevel::new();
    }

    {
        let obj = TestToplevelNestedPublic::new();

        let t = 0;
        let e: TestToplevelNestedPublicE = TestToplevelNestedPublicE::V;
        TestToplevel::func(&obj, t, e);
    }

    {
        let _obj = TestToplevelNestedPublicNestedNestedPublic::new();
    }

    }
}
