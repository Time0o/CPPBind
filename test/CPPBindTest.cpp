#define CATCH_CONFIG_RUNNER

#include "catch2/catch.hpp"

#include "Options.hpp"
#include "Wrappable.hpp"
#include "WrappedBy.hpp"
#include "WrapperTest.hpp"

using namespace cppbind_test;

TEST_CASE("Functions")
{
  WrapperTest("simplest possible function",
    Wrappable(
      "void foo();"),
    WrappedBy(
      "void test_foo()\n"
      "{ test::foo(); }"));

  WrapperTest("function with arguments",
    Wrappable(
      "int foo(int a, int b);"),
    WrappedBy(
      "int test_foo(int a, int b)\n"
      "{ return test::foo(a, b); }"));

  WrapperTest("function with unnamed arguments",
    Wrappable(
      "int foo(int a, int, int b, int);"),
    WrappedBy(
      "int test_foo(int a, int _2, int b, int _4)\n"
      "{ return test::foo(a, _2, b, _4); }"));

  cppbind::Options().set<bool>("overload-default-params", false);

  WrapperTest("function with default arguments",
     Wrappable(
       "int foo(int a, int b = 1, bool c = true, double d = 0.5, void *e = nullptr);"),
     WrappedBy(
       "int test_foo(int a, int b, bool c, double d, void * e)\n"
       "{ return test::foo(a, b, c, d, e); }"));

  cppbind::Options().set<bool>("overload-default-params", true);

  WrapperTest("function with default arguments (overloading)",
     Wrappable(
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

  WrapperTest("function with lvalue reference parameters",
     Wrappable(
       "int & foo(int &a, int const &b);"),
     WrappedBy(
       "int * test_foo(int * a, const int * b)\n"
       "{ return &test::foo(*a, *b); }"));

  WrapperTest("function with rvalue reference parameters",
     Wrappable(
       "void foo(int &&a, int const &&b);"),
     WrappedBy(
       "void test_foo(int a, const int b)\n"
       "{ test::foo(std::move(a), std::move(b)); }")
       .mustInclude("utility", true));

  // XXX pass structure types (also by pointer, ref, handle move and copy construction)

  WrapperTest("function overloads",
     Wrappable(
       "void foo(int a);\n"
       "void foo(double a);"),
     WrappedBy(
       "void test_foo_1(int a);\n"
       "void test_foo_2(double a);"));

  // XXX template functions

  // XXX more cases
  WrapperTest("function naming conflicts",
     Wrappable(
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
    Wrappable(
      "char32_t foo(const char16_t * a);"),
    WrappedBy(
      "uint32_t test_foo(const uint16_t * a)\n"
      "{ return static_cast<uint32_t>(test::foo(static_cast<const char16_t *>(a))); }")
      .mustInclude("stdint.h", true));

  WrapperTest("fundamental type includes",
    Wrappable(
      "void foo(bool a, wchar_t b);"),
    WrappedBy(
      "void test_foo(bool a, wchar_t b);")
      .mustInclude("bool.h", true)
      .mustInclude("wchar.h", true));

  WrapperTest("typedefs resolution",
    Wrappable(
      "typedef int INT;\n"
      "using FLOAT = double;\n"
      "INT round(FLOAT f);"),
    WrappedBy(
      "int test_round(double f);"));
}

int main(int argc, char **argv)
{ return wrapperTestMain(argc, argv); }
