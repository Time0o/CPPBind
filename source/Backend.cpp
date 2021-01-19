#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "pybind11/embed.h"
#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

#include "Backend.hpp"
#include "Error.hpp"
#include "Identifier.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "Path.hpp"
#include "Wrapper.hpp"
#include "WrapperVariable.hpp"
#include "WrapperFunction.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"

namespace py = pybind11;

namespace cppbind
{

auto Backend::importModule(std::string const &Module)
{ return py::module::import(Module.c_str()); }

auto Backend::addModuleSearchPath(std::string const &Path)
{
  static auto Sys = importModule("sys");

  Sys.attr("path").cast<py::list>().append(Path);
}

void Backend::run(std::shared_ptr<Wrapper> Wrapper)
{
  auto BE = Options().get<>("backend");

  py::scoped_interpreter Guard;

  try {
    addModuleSearchPath(BACKEND_IMPL_COMMON_DIR);
    addModuleSearchPath(path::concat(BACKEND_IMPL_DIR, BE));

    auto Module(importModule(BE + BACKEND_IMPL_MODULE_POSTFIX));
    auto ModuleEntry(Module.attr(BACKEND_IMPL_ENTRY));

    ModuleEntry(Wrapper, &Options());

  } catch (std::runtime_error const &e) {
    throw CPPBindError(ErrorMsg() << "in backend:" << ErrorMsg::endl << e.what());
  }
}

} // namespace cppbind

PYBIND11_EMBEDDED_MODULE(cppbind, m)
{
  using namespace cppbind;
  using namespace py::literals;

  auto str = [](Identifier const &Id,
                std::string const &Case_,
                std::string const &Qualifiers)
  {
    Identifier::Case Case;

    if (Case_ == "orig")
      Case = Identifier::ORIG_CASE;
    else if (Case_ == "camel")
      Case = Identifier::CAMEL_CASE;
    else if (Case_ == "pascal")
      Case = Identifier::PASCAL_CASE;
    else if (Case_ == "snake")
      Case = Identifier::SNAKE_CASE;
    else if (Case_ == "snake-cap-first")
      Case = Identifier::SNAKE_CASE_CAP_FIRST;
    else if (Case_ == "snake-cap-all")
      Case = Identifier::SNAKE_CASE_CAP_ALL;
    else
      throw std::invalid_argument("invalid argument to 'case'");

    if (Qualifiers == "keep")
      return Id.str(Case);
    else if (Qualifiers == "replace")
      return Id.unscoped().str(Case);
    else if (Qualifiers == "remove")
      return Id.unqualified().str(Case);
    else
      throw std::invalid_argument("invalid argument to 'qualifiers'");
  };

  #define STR(T, FUNC) [&str](T const &Self, \
                              std::string const &Case, \
                              std::string const &Qualifiers) \
                             { return str(Self.FUNC(), Case, Qualifiers); }, \
                             "case"_a = "orig", "qualifiers"_a = "keep"

  py::class_<Wrapper, std::shared_ptr<Wrapper>>(m, "Wrapper")
    .def("wrapped_file", &Wrapper::wrappedFile)
    .def("variables", &Wrapper::constants)
    .def("records", &Wrapper::records)
    .def("functions", &Wrapper::functions);

  py::class_<WrapperType>(m, "WrapperType")
    .def(py::self == py::self)
    .def(py::self != py::self)
    .def("__str__", &WrapperType::str)
    .def("is_const", &WrapperType::isConst)
    .def("is_fundamental", &WrapperType::isFundamental, "which"_a = nullptr)
    .def("is_void", &WrapperType::isVoid)
    .def("is_integral", &WrapperType::isIntegral)
    .def("is_floating", &WrapperType::isFloating)
    .def("is_reference", &WrapperType::isReference)
    .def("is_lvalue_reference", &WrapperType::isLValueReference)
    .def("is_rvalue_reference", &WrapperType::isRValueReference)
    .def("is_pointer", &WrapperType::isPointer)
    .def("is_class", &WrapperType::isClass)
    .def("is_struct", &WrapperType::isStruct)
    .def("base", &WrapperType::base)
    .def("reference_to", &WrapperType::referenceTo)
    .def("referenced", &WrapperType::referenced)
    .def("pointer_to", &WrapperType::pointerTo)
    .def("pointee", &WrapperType::pointee, "recursive"_a = false)
    .def("with_const", &WrapperType::withConst)
    .def("without_const", &WrapperType::withoutConst);

  py::class_<WrapperVariable>(m, "WrapperVariable")
    .def("name", STR(WrapperVariable, name))
    .def("type", &WrapperVariable::type);

  py::class_<WrapperRecord>(m, "WrapperRecord");

  auto WrapperParam_ = py::class_<WrapperParam>(m, "WrapperParam")
    .def("name", STR(WrapperParam, name))
    .def("type", &WrapperParam::type)
    .def("has_default_argument", &WrapperParam::hasDefaultArg)
    .def("default_argument",
         [](WrapperParam const &Self) -> std::optional<WrapperParam::DefaultArg>
         {
            if (!Self.hasDefaultArg())
              return std::nullopt;

            return Self.defaultArg();
         });

  py::class_<WrapperParam::DefaultArg>(WrapperParam_, "DefaultArgument")
    .def("__str__", &WrapperParam::DefaultArg::str)
    .def("is_int", &WrapperParam::DefaultArg::isInt)
    .def("is_float", &WrapperParam::DefaultArg::isFloat)
    .def("is_true", &WrapperParam::DefaultArg::isTrue);

  py::class_<WrapperFunction>(m, "WrapperFunction")
    .def("name", STR(WrapperFunction, name))
    .def("overload_name", STR(WrapperFunction, overloadName))
    .def("return_type", &WrapperFunction::returnType)
    .def("num_parameters",
         [](WrapperFunction const &Self, bool RequiredOnly)
         {
           if (RequiredOnly)
             return Self.numParamsRequired();

           return Self.numParams();
         },
         "required_only"_a = false)
    .def("parameters", &WrapperFunction::params);

  #define OPTION(NAME) [](OptionsRegistry const &Self) \
                       { return Self.get<>(NAME); }

  py::class_<OptionsRegistry>(m, "Options")
    .def("output-directory", OPTION("output-directory"));
}
