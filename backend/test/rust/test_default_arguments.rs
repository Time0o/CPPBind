mod test_default_arguments_rust;
use test_default_arguments_rust as test;

fn main() {
    unsafe {

    assert!(test::test_pow_default_arguments(test::get_test_e(), 2, 1, test::TestRound::Upward) == 8.0);

    assert!(test::test_default_large_signed(test::LARGE_SIGNED) == test::LARGE_SIGNED);
    assert!(test::test_default_constexpr_function_call(test::test_func_constexpr()) == test::test_func_constexpr());
    assert!(test::test_default_function_call(test::test_func()) == test::test_func());


    }
}
