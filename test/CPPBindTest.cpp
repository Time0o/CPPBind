#define CATCH_CONFIG_RUNNER

#include "catch2/catch.hpp"

#include "CPPBindTestHelpers.hpp"

using namespace cppbind_test;

TEST_CASE("Functions")
{
  WrapperTest("simplest possible function",
    makeWrappable(
      "void foo();"),
    WrappedBy(
      "void test_foo()\n"
      "{ test::foo(); }"));

  WrapperTest("function with arguments",
    makeWrappable(
      "int foo(int a, int b);"),
    WrappedBy(
      "int test_foo(int a, int b)\n"
      "{ return test::foo(a, b); }"));

  WrapperTest("function with unnamed arguments",
    makeWrappable(
      "int foo(int a, int, int b, int);"),
    WrappedBy(
      "int test_foo(int a, int _2, int b, int _4)\n"
      "{ return test::foo(a, _2, b, _4); }"));

  WrapperTest("function with default arguments",
     makeWrappable(
       "int foo(int a, int b = 1, bool c = true, double d = 0.5, void *e = nullptr);"),
     WrappedBy(
       "int test_foo_1(int a)\n"
       "{ return test::foo(a, 1, true, 0.5, NULL); }\n"
       "int test_foo_2(int a, int b)\n"
       "{ return test::foo(a, b, true, 0.5, NULL); }\n"
       "int test_foo_3(int a, int b, bool c)\n"
       "{ return test::foo(a, b, c, 0.5, NULL); }\n"
       "int test_foo_4(int a, int b, bool c, double d)\n"
       "{ return test::foo(a, b, c, d, NULL); }\n"
       "int test_foo_5(int a, int b, bool c, double d, void * e)\n"
       "{ return test::foo(a, b, c, d, e); }"));

  // XXX variadic macros/varargs

  WrapperTest("function with lvalue reference parameters",
     makeWrappable(
       "int & foo(int &a, int const &b);"),
     WrappedBy(
       "int * test_foo(int * a, const int * b)\n"
       "{ return &test::foo(*a, *b); }"));

  WrapperTest("function with rvalue reference parameters",
     makeWrappable(
       "void foo(int &&a, int const &&b);"),
     WrappedBy(
       "void test_foo(int a, const int b)\n"
       "{ test::foo(std::move(a), std::move(b)); }",
       WrapperIncludes({"utility"})));

  // XXX pass structure types (also by pointer, ref, handle move and copy construction)

  WrapperTest("function overloads",
     makeWrappable(
       "void foo(int a);\n"
       "void foo(double a);"),
     WrappedBy(
       "void test_foo_1(int a);\n"
       "void test_foo_2(double a);"));

  // XXX template functions

  // XXX more cases
  WrapperTest("function naming conflicts",
     makeWrappable(
       "int foo(int a);\n"
       "int foo(double a);\n"
       "int foo_1();\n"
       "int foo_1_();"),
     WrappedBy(
       "int test_foo_1__(int a);\n"
       "int test_foo_2(double a);\n"
       "int test_foo_1();\n"
       "int test_foo_1_();"));
}

TEST_CASE("Types")
{
  WrapperTest("fundamental type conversion",
    makeWrappable(
      "char32_t foo(const char16_t * a);"),
    WrappedBy(
      "uint32_t test_foo(const uint16_t * a)\n"
      "{ return static_cast<uint32_t>(test::foo(static_cast<const char16_t *>(a))); }",
      WrapperIncludes({"stdint.h"})));

  WrapperTest("fundamental type includes",
    makeWrappable(
      "void foo(bool a, wchar_t b);"),
    WrappedBy(
      "void test_foo(bool a, wchar_t b);",
      WrapperIncludes({"bool.h", "wchar.h"})));

  WrapperTest("typedefs resolution",
    makeWrappable(
      "typedef int INT;\n"
      "using FLOAT = double;\n"
      "INT round(FLOAT f);"),
    WrappedBy(
      "int test_round(double f);"));
}

int main(int argc, char **argv)
{ return wrapperTestMain(argc, argv); }
