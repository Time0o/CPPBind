#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "pybind11/embed.h"
#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"

#include "Backend.hpp"
#include "Identifier.hpp"
#include "Include.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "Wrapper.hpp"
#include "WrapperConstant.hpp"
#include "WrapperFunction.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

pybind11::module_ Backend::importModule(std::string const &Module)
{ return pybind11::module_::import(Module.c_str()); }

void Backend::addModuleSearchPath(pybind11::module_ const &Sys,
                                  std::string const &Path)
{ Sys.attr("path").cast<pybind11::list>().append(Path); }

void Backend::run(std::string const &InputFile,
                  std::shared_ptr<Wrapper> Wrapper)
{
  using namespace boost::filesystem;

  auto BE = OPT("backend");

  pybind11::scoped_interpreter Guard;

  try {
    auto Sys(importModule("sys"));
    addModuleSearchPath(Sys, BACKEND_IMPL_COMMON_DIR);

    addModuleSearchPath(Sys, (path(BACKEND_IMPL_DIR) / BE).string());

    auto BackendModule(importModule(BE + BACKEND_IMPL_BACKEND_MOD_POSTFIX));

    auto RunModule(importModule(BACKEND_IMPL_RUN_MOD));
    RunModule.attr("run")(InputFile, Wrapper);

  } catch (std::runtime_error const &e) {
    throw log::exception("in backend:\n{0}", e.what());
  }
}

} // namespace cppbind

using namespace cppbind;

using Type = WrapperType;
using Constant = WrapperConstant;
using Function = WrapperFunction;
using Parameter = WrapperParameter;
using Record = WrapperRecord;

PYBIND11_EMBEDDED_MODULE(cppbind, m)
{
  namespace py = pybind11;

  using namespace py::literals;

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
    .def_readonly_static("OUT", &Identifier::OUT)
    .def_readonly_static("NEW", &Identifier::NEW)
    .def_readonly_static("DELETE", &Identifier::DELETE)
    .def_readonly_static("COPY", &Identifier::COPY)
    .def_readonly_static("MOVE", &Identifier::MOVE)
    .def_readonly_static("COPY_ASSIGN", &Identifier::COPY_ASSIGN)
    .def_readonly_static("MOVE_ASSIGN", &Identifier::MOVE_ASSIGN)
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

  py::class_<Include>(m, "Include")
    .def(py::init<std::string, bool>(), "path"_a, "system"_a = false)
    .def("__str__", [](Include const &Self){ return Self.str(); })
    .def("str", &Include::str, "relative"_a = false)
    .def("path", &Include::path);

  py::class_<Wrapper, std::shared_ptr<Wrapper>>(m, "Wrapper")
    .def("includes", &Wrapper::getIncludes)
    .def("constants", &Wrapper::getConstants,
         py::return_value_policy::reference_internal)
    .def("functions", &Wrapper::getFunctions,
         py::return_value_policy::reference_internal)
    .def("records", &Wrapper::getRecords,
         py::return_value_policy::reference_internal);

  py::class_<Type>(m, "Type", py::dynamic_attr())
    .def(py::init<std::string>(), "which"_a = "void")
    .def(py::self == py::self)
    .def(py::self != py::self)
    .def(hash(py::self))
    .def("__str__", [](WrapperType const &Self){ return Self.str(); })
    .def("str", &WrapperType::str,
         "with_template_postfix"_a = false)
    .def("format", &WrapperType::format,
         "with_template_postfix"_a = false,
         "with_prefix"_a = "",
         "with_postfix"_a = "",
         "case"_a = Identifier::ORIG_CASE,
         "quals"_a = Identifier::KEEP_QUALS)
    .def("mangled", &WrapperType::mangled)
    .def("is_const", &WrapperType::isConst)
    .def("is_fundamental", &WrapperType::isFundamental, "which"_a = nullptr)
    .def("is_void", &WrapperType::isVoid)
    .def("is_boolean", &WrapperType::isBoolean)
    .def("is_enum", &WrapperType::isEnum)
    .def("is_scoped_enum", &WrapperType::isScopedEnum)
    .def("is_integral", &WrapperType::isIntegral)
    .def("is_floating", &WrapperType::isFloating)
    .def("is_c_string", &WrapperType::isCString)
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
    .def("underlying_integer_type", &WrapperType::underlyingIntegerType)
    .def("qualifiers", &WrapperType::qualifiers)
    .def("qualified", &WrapperType::qualified, "qualifiers"_a = 0u)
    .def("unqualified", &WrapperType::unqualified)
    .def("with_const", &WrapperType::withConst)
    .def("without_const", &WrapperType::withoutConst)
    .def("size", &WrapperType::size);

  py::implicitly_convertible<std::string, Type>();

  py::class_<Constant>(m, "Constant", py::dynamic_attr())
    .def("name", &WrapperConstant::getName)
    .def("type", &WrapperConstant::getType);

  py::class_<Function>(m, "Function", py::dynamic_attr())
    .def("name", &WrapperFunction::getName, "with_template_postfix"_a = false,
                                            "with_overload_postfix"_a = false,
                                            "without_operator_name"_a = false)
    .def("enclosing_namespaces", &WrapperFunction::getEnclosingNamespaces)
    .def("return_type", &Function::getReturnType)
    .def("out_type", &Function::getOutType)
    .def("parameters",
         py::overload_cast<>(&WrapperFunction::getParameters, py::const_),
         py::return_value_policy::reference_internal)
    .def("parent", &WrapperFunction::getParent)
    .def("overloaded_operator", &WrapperFunction::getOverloadedOperator)
    .def("template_argument_list", &WrapperFunction::getTemplateArgumentList)
    .def("is_member", &WrapperFunction::isMember)
    .def("is_instance", &WrapperFunction::isInstance)
    .def("is_constructor", &WrapperFunction::isConstructor)
    .def("is_destructor", &WrapperFunction::isDestructor)
    .def("is_static", &WrapperFunction::isStatic)
    .def("is_const", &WrapperFunction::isConst)
    .def("is_noexcept", &WrapperFunction::isNoexcept)
    .def("is_overloaded_operator", &WrapperFunction::isOverloadedOperator)
    .def("is_template_instantiation", &WrapperFunction::isTemplateInstantiation);

  py::class_<Parameter>(m, "Parameter")
    .def("name", &WrapperParameter::getName)
    .def("type", &WrapperParameter::getType)
    .def("default_argument", &WrapperParameter::getDefaultArgument)
    .def("is_self", &WrapperParameter::isSelf)
    .def("is_out", &WrapperParameter::isOut);

  auto PyRecord = py::class_<Record>(m, "Record", py::dynamic_attr())
    .def("name", &WrapperRecord::getName, "with_template_postfix"_a = false)
    .def("type", &WrapperRecord::getType)
    .def("functions",
         py::overload_cast<>(&WrapperRecord::getFunctions, py::const_),
         py::return_value_policy::reference_internal)
    .def("template_argument_list", &WrapperRecord::getTemplateArgumentList)
    .def("is_abstract", &WrapperRecord::isAbstract)
    .def("is_copyable", &WrapperRecord::isCopyable)
    .def("is_moveable", &WrapperRecord::isMoveable);

  #define RO_PROP(NAME, VALUE) \
    .def_property_readonly_static(NAME, [](py::object const &){ return VALUE; })

  struct Options_ {};

  py::class_<Options_>(m, "Options")
    RO_PROP("output_noexcept", OPT(bool, "output-noexcept"))
    RO_PROP("output_directory", OPT("output-directory"))
    RO_PROP("output_c_header_extension", OPT("output-c-header-extension"))
    RO_PROP("output_c_source_extension", OPT("output-c-source-extension"))
    RO_PROP("output_cpp_header_extension", OPT("output-cpp-header-extension"))
    RO_PROP("output_cpp_source_extension", OPT("output-cpp-source-extension"))
    RO_PROP("output_relative_includes", OPT(bool, "output-relative-includes"))
    RO_PROP("lua_include_dir", OPT("lua-include-dir"))
    RO_PROP("lua_include_cpp", OPT(bool, "lua-include-cpp"));
}
