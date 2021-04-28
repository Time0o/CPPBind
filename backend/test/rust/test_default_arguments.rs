mod test_default_arguments_rust;
use test_default_arguments_rust::*;

fn main() {
    unsafe {

    assert!(test::pow_default_arguments(test::get_e(), 2, 1, test::Round::Upward) == 8.0);

    assert!(test::default_large_signed(test::LARGE_SIGNED) == test::LARGE_SIGNED);
    assert!(test::default_constexpr_function_call(test::func_constexpr()) == test::func_constexpr());
    assert!(test::default_function_call(test::func()) == test::func());

    }
}
