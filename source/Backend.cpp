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
  auto BE = OPT("backend");

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

  py::class_<Identifier> PyIdentifier(m, "Identifier");

  py::enum_<Identifier::Case>(PyIdentifier, "Case")
    .value("ORIG_CASE", Identifier::ORIG_CASE)
    .value("CAMEL_CASE", Identifier::CAMEL_CASE)
    .value("PASCAL_CASE", Identifier::PASCAL_CASE)
    .value("SNAKE_CASE", Identifier::SNAKE_CASE)
    .value("SNAKE_CASE_CAP_FIRST", Identifier::SNAKE_CASE_CAP_FIRST)
    .value("SNAKE_CASE_CAP_ALL", Identifier::SNAKE_CASE_CAP_ALL)
    .export_values();

  py::enum_<Identifier::Quals>(PyIdentifier, "Quals")
    .value("KEEP_QUALS", Identifier::KEEP_QUALS)
    .value("REMOVE_QUALS", Identifier::REMOVE_QUALS)
    .value("REPLACE_QUALS", Identifier::REPLACE_QUALS)
    .export_values();

  PyIdentifier
    .def_readonly_static("SELF", &Identifier::SELF)
    .def_readonly_static("NEW", &Identifier::NEW)
    .def_readonly_static("DELETE", &Identifier::DELETE)
    .def(py::init<std::string>())
    .def("__str__", &Identifier::str)
    .def("format", &Identifier::format,
         "case"_a = Identifier::ORIG_CASE,
         "quals"_a = Identifier::KEEP_QUALS);

  py::implicitly_convertible<std::string, Identifier>();

  py::class_<Wrapper, std::shared_ptr<Wrapper>>(m, "Wrapper")
    .def("wrapped_file", &Wrapper::wrappedFile)
    .def("variables", &Wrapper::constants)
    .def("records", &Wrapper::records)
    .def("functions", &Wrapper::functions);

  py::class_<WrapperType>(m, "Type")
    .def(py::init<std::string>(), "which"_a = "void")
    .def(py::self == py::self)
    .def(py::self != py::self)
    .def("__str__", &WrapperType::str, "compact"_a = false)
    .def("format", &WrapperType::format,
         "case"_a = Identifier::ORIG_CASE,
         "quals"_a = Identifier::KEEP_QUALS)
    .def("is_const", &WrapperType::isConst)
    .def("is_fundamental", &WrapperType::isFundamental, "which"_a = nullptr)
    .def("is_void", &WrapperType::isVoid)
    .def("is_boolean", &WrapperType::isBoolean)
    .def("is_enum", &WrapperType::isEnum)
    .def("is_scoped_enum", &WrapperType::isScopedEnum)
    .def("is_integral", &WrapperType::isIntegral)
    .def("is_integral_underlying_enum", &WrapperType::isIntegralUnderlyingEnum)
    .def("is_floating", &WrapperType::isFloating)
    .def("is_reference", &WrapperType::isReference)
    .def("is_lvalue_reference", &WrapperType::isLValueReference)
    .def("is_rvalue_reference", &WrapperType::isRValueReference)
    .def("is_pointer", &WrapperType::isPointer)
    .def("is_record", &WrapperType::isRecord)
    .def("is_class", &WrapperType::isClass)
    .def("is_struct", &WrapperType::isStruct)
    .def("lvalue_reference_to", &WrapperType::lvalueReferenceTo)
    .def("rvalue_reference_to", &WrapperType::rvalueReferenceTo)
    .def("referenced", &WrapperType::referenced)
    .def("pointer_to", &WrapperType::pointerTo)
    .def("pointee", &WrapperType::pointee, "recursive"_a = false)
    .def("qualified", &WrapperType::qualified, "qualifiers"_a = 0u)
    .def("unqualified", &WrapperType::unqualified)
    .def("with_const", &WrapperType::withConst)
    .def("without_const", &WrapperType::withoutConst)
    .def("with_enum", &WrapperType::withEnum)
    .def("without_enum", &WrapperType::withoutEnum)
    .def_property("base", &WrapperType::base, &WrapperType::changeBase);

  py::class_<WrapperVariable>(m, "Variable")
    .def(py::init<Identifier, WrapperType>(), "name"_a, "type"_a)
    .def_property_readonly("name", &WrapperVariable::name)
    .def_property_readonly("type", &WrapperVariable::type);

  py::class_<WrapperRecord>(m, "Record")
    .def(py::init<WrapperType>(), "type"_a)
    .def_property_readonly("name", &WrapperRecord::name)
    .def_property_readonly("type", &WrapperRecord::type);

  auto PyWrapperParameter = py::class_<WrapperParameter>(m, "Parameter")
    .def(py::init<Identifier, WrapperType>(), "name"_a, "type"_a)
    .def_property_readonly("name", &WrapperParameter::name)
    .def_property_readonly("type", &WrapperParameter::type)
    .def("default_argument",
         [](WrapperParameter const &Self)
           -> std::optional<WrapperParameter::DefaultArgument>
         {
            if (!Self.hasDefaultArgument())
              return std::nullopt;

            return Self.defaultArgument();
         });

  py::class_<WrapperParameter::DefaultArgument>(PyWrapperParameter, "DefaultArgument")
    .def("__str__", &WrapperParameter::DefaultArgument::str)
    .def("is_int", &WrapperParameter::DefaultArgument::isInt)
    .def("is_float", &WrapperParameter::DefaultArgument::isFloat)
    .def("is_true", &WrapperParameter::DefaultArgument::isTrue);

  py::class_<WrapperFunction>(m, "Function")
    .def_property_readonly("name", &WrapperFunction::name)
    .def_property_readonly("name_overloaded", &WrapperFunction::nameOverloaded)
    .def_property_readonly("self_type", &WrapperFunction::selfType)
    .def_property_readonly("return_type", &WrapperFunction::returnType)
    .def("is_member", &WrapperFunction::isMember)
    .def("is_constructor", &WrapperFunction::isConstructor)
    .def("is_destructor", &WrapperFunction::isDestructor)
    .def("is_static", &WrapperFunction::isStatic)
    .def("is_overloaded", &WrapperFunction::isOverloaded)
    .def("parameters", &WrapperFunction::parameters, "required_only"_a = false);

  py::class_<WrapperFunctionBuilder>(m, "FunctionBuilder")
    .def(py::init<Identifier>(), "name"_a)
    .def("set_self_type", &WrapperFunctionBuilder::setSelfType, "type"_a)
    .def("set_return_type", &WrapperFunctionBuilder::setReturnType, "type"_a)
    .def("add_parameter", &WrapperFunctionBuilder::addParameter, "param"_a)
    .def("set_is_constructor", &WrapperFunctionBuilder::setIsConstructor)
    .def("set_is_destructor", &WrapperFunctionBuilder::setIsDestructor)
    .def("set_is_static", &WrapperFunctionBuilder::setIsStatic)
    .def("build", &WrapperFunctionBuilder::build);

  #define GET_OPT(NAME) [](OptionsRegistry const &Self) \
                        { return Self.get<>(NAME); }

  py::class_<OptionsRegistry>(m, "Options")
    .def("output-directory", GET_OPT("output-directory"));
}
