#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "pybind11/embed.h"
#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"

#include "Backend.hpp"
#include "Error.hpp"
#include "Identifier.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "Path.hpp"
#include "Snippet.hpp"
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

    auto TypetransModule(importModule(BE + BACKEND_IMPL_TYPETRANS_MOD_POSTFIX));
    auto BackendModule(importModule(BE + BACKEND_IMPL_BACKEND_MOD_POSTFIX));

    auto RunModule(importModule(BACKEND_IMPL_RUN_MOD));
    RunModule.attr("run")(Wrapper, &Options());

  } catch (std::runtime_error const &e) {
    throw CPPBindError(ErrorMsg() << "in backend:" << ErrorMsg::endl << e.what());
  }
}

} // namespace cppbind

using namespace cppbind;

using Type = WrapperType;
using Variable = WrapperVariable;
using Function = WrapperFunction;
using Parameter = WrapperParameter;
using DefaultArgument = WrapperDefaultArgument;
using Record = WrapperRecord;

PYBIND11_MAKE_OPAQUE(std::vector<WrapperVariable>);
PYBIND11_MAKE_OPAQUE(std::vector<WrapperFunction>);
PYBIND11_MAKE_OPAQUE(std::vector<WrapperParameter>);
PYBIND11_MAKE_OPAQUE(std::vector<WrapperRecord>);

PYBIND11_EMBEDDED_MODULE(cppbind, m)
{
  using namespace py::literals;

  py::bind_vector<std::vector<WrapperVariable>>(m, "VectorVariables");
  py::bind_vector<std::vector<WrapperFunction>>(m, "VectorFunctions");
  py::bind_vector<std::vector<WrapperParameter>>(m, "VectorParameters");
  py::bind_vector<std::vector<WrapperRecord>>(m, "VectorRecords");

  auto PyIdentifier = py::class_<Identifier>(m, "Identifier");

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
    .def_readonly_static("RET", &Identifier::RET)
    .def_readonly_static("NEW", &Identifier::NEW)
    .def_readonly_static("DELETE", &Identifier::DELETE)
    .def(py::init<std::string>())
    .def(py::self == py::self)
    .def(py::self != py::self)
    .def(py::self < py::self)
    .def(py::self > py::self)
    .def(py::self <= py::self)
    .def(py::self >= py::self)
    .def("__str__", &Identifier::str)
    .def("qualified", &Identifier::qualified, "qualifiers"_a)
    .def("unqualified", &Identifier::unqualified)
    .def("unscoped", &Identifier::unscoped)
    .def("str", &Identifier::str)
    .def("format", &Identifier::format,
         "case"_a = Identifier::ORIG_CASE,
         "quals"_a = Identifier::KEEP_QUALS);

  py::implicitly_convertible<std::string, Identifier>();

  py::class_<Wrapper, std::shared_ptr<Wrapper>>(m, "Wrapper")
    .def("input_file", &Wrapper::inputFile)
    .def("variables",
         &Wrapper::variables,
         py::return_value_policy::reference_internal)
    .def("functions",
         &Wrapper::functions,
         py::return_value_policy::reference_internal)
    .def("records",
         &Wrapper::records,
         py::return_value_policy::reference_internal);

  py::class_<Type>(m, "Type", py::dynamic_attr())
    .def(py::init<std::string>(), "which"_a = "void")
    .def(py::self == py::self)
    .def(py::self != py::self)
    .def(hash(py::self))
    .def("__str__", &WrapperType::str)
    .def("str", &WrapperType::str)
    .def("format", &WrapperType::format,
         "mangled"_a = false,
         "compact"_a = false,
         "case"_a = Identifier::ORIG_CASE,
         "quals"_a = Identifier::KEEP_QUALS)
    .def_property("base", &WrapperType::getBase, &WrapperType::setBase)
    .def_property("annotations", &WrapperType::getAnnotations, &WrapperType::setAnnotations)
    .def("is_const", &WrapperType::isConst)
    .def("is_matched_by", &WrapperType::isMatchedBy, "matcher"_a)
    .def("is_fundamental", &WrapperType::isFundamental, "which"_a = nullptr)
    .def("is_void", &WrapperType::isVoid)
    .def("is_boolean", &WrapperType::isBoolean)
    .def("is_enum", &WrapperType::isEnum)
    .def("is_scoped_enum", &WrapperType::isScopedEnum)
    .def("is_integral", &WrapperType::isIntegral)
    .def("is_integral_underlying_enum", &WrapperType::isIntegralUnderlyingEnum)
    .def("is_integral_underlying_scoped_enum", &WrapperType::isIntegralUnderlyingScopedEnum)
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
    .def("pointer_to", &WrapperType::pointerTo, "repeat"_a = 0u)
    .def("pointee", &WrapperType::pointee, "recursive"_a = false)
    .def("qualifiers", &WrapperType::qualifiers)
    .def("qualified", &WrapperType::qualified, "qualifiers"_a = 0u)
    .def("unqualified", &WrapperType::unqualified)
    .def("with_const", &WrapperType::withConst)
    .def("without_const", &WrapperType::withoutConst)
    .def("with_enum", &WrapperType::withEnum)
    .def("without_enum", &WrapperType::withoutEnum);

  py::class_<Variable>(m, "Variable", py::dynamic_attr())
    .def(py::init<Identifier, WrapperType>(), "name"_a, "type"_a)
    .def_property("name", &WrapperVariable::getName,
                          &WrapperVariable::setName)
    .def_property("type", &WrapperVariable::getType,
                          &WrapperVariable::setType);

  py::class_<Function>(m, "Function", py::dynamic_attr())
    .def("name", &WrapperFunction::getName, "overloaded"_a = false)
    .def("parent", &WrapperFunction::getParent)
    .def("return_type", &WrapperFunction::getReturnType)
    .def("parameters", &WrapperFunction::getParameters, "skip_self"_a = false)
    .def("is_member", &WrapperFunction::isMember)
    .def("is_instance", &WrapperFunction::isInstance)
    .def("is_constructor", &WrapperFunction::isConstructor)
    .def("is_destructor", &WrapperFunction::isDestructor)
    .def("is_static", &WrapperFunction::isStatic)
    .def("is_const", &WrapperFunction::isConst)
    .def("is_noexcept", &WrapperFunction::isNoexcept);

  auto PyRecord = py::class_<Record>(m, "Record", py::dynamic_attr())
    .def_property_readonly("name", &WrapperRecord::getName)
    .def_property_readonly("type", &WrapperRecord::getType)
    .def_property_readonly("constructors", &WrapperRecord::getConstructors)
    .def_property_readonly("destructor", &WrapperRecord::getDestructor)
    .def_readwrite("variables", &WrapperRecord::Variables)
    .def_readwrite("functions", &WrapperRecord::Functions)
    .def("is_abstract", &WrapperRecord::isAbstract)
    .def("is_copyable", &WrapperRecord::isCopyable)
    .def("is_moveable", &WrapperRecord::isMoveable);

  py::class_<Parameter>(m, "Parameter")
    // XXX pass default argument
    .def(py::init<Identifier, WrapperType>(), "name"_a, "type"_a)
    .def_static("self", &WrapperParameter::self, "type"_a)
    .def_property("name", &WrapperParameter::getName,
                          &WrapperParameter::setName)
    .def_property("type", &WrapperParameter::getType,
                          &WrapperParameter::setType)
    .def_property("default_argument", &WrapperParameter::getDefaultArgument,
                                      &WrapperParameter::setDefaultArgument)
    .def("is_self", &WrapperParameter::isSelf);

  py::class_<DefaultArgument>(m, "DefaultArgument")
    // XXX constructor
    .def("__str__", &WrapperDefaultArgument::str)
    .def("str", &WrapperDefaultArgument::str)
    .def("is_int", &WrapperDefaultArgument::isInt)
    .def("is_float", &WrapperDefaultArgument::isFloat)
    .def("is_true", &WrapperDefaultArgument::isTrue);

  #define GET_OPT(NAME) [](OptionsRegistry const &Self) \
                        { return Self.get<>(NAME); }

  py::class_<OptionsRegistry>(m, "Options")
    .def("output-directory", GET_OPT("output-directory"));
}
