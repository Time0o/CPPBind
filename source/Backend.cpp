#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "pybind11/embed.h"
#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"

#include "Backend.hpp"
#include "Env.hpp"
#include "Identifier.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "Wrapper.hpp"
#include "WrapperFunction.hpp"
#include "WrapperInclude.hpp"
#include "WrapperMacro.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"
#include "WrapperVariable.hpp"

namespace fs = std::filesystem;

// Add a directory to the Python module search path.
static void
addModuleSearchPath(pybind11::module const &SysMod, std::string const &Path)
{ SysMod.attr("path").cast<pybind11::list>().append(Path.c_str()); }

// Import some Python module.
static pybind11::module
importModule(std::string const &Module)
{ return pybind11::module::import(Module.c_str()); }

namespace cppbind
{

namespace backend
{

void run(std::string const &InputFile, std::shared_ptr<Wrapper> Wrapper)
{
  // This starts up the Python interpreter and consequently shuts it down when
  // it goes out of scope.
  pybind11::scoped_interpreter Guard;

  try {
    // Backend root directory.
    auto SysMod(importModule("sys"));

    addModuleSearchPath(SysMod, BACKEND_IMPL_COMMON_DIR);

    // List of backends to initialize. Includes both the C backend and the
    // backend passed by the user via the '--backend' option (if different).
    // The former is always initialized in order to allow the latter to
    // generate 'intermediate' C bindings if necessary (as is e.g. the case for
    // Rust).
    std::string CBackend("c");
    std::string TargetBackend(OPT("backend"));

    std::vector<std::string> Backends { CBackend };

    if (TargetBackend != CBackend)
      Backends.push_back(TargetBackend);

    // Initialize and run backend(s).
    for (auto const &Backend : Backends) {
      addModuleSearchPath(SysMod, (fs::path(BACKEND_IMPL_DIR) / Backend).string());

      importModule(Backend + "_backend");
    }

    auto BackendMod(importModule("backend"));

    for (auto const &Backend : Backends)
      BackendMod.attr("initialize_backend")(Backend, InputFile, Wrapper);

    BackendMod.attr("run_backend")(TargetBackend);

  } catch (std::runtime_error const &e) {
    throw log::exception("in backend:\n{0}", e.what());
  }
}

} // namespace backend

} // namespace cppbind

using namespace cppbind;

using Type = WrapperType;
using Include = WrapperInclude;
using Macro = WrapperMacro;
using Enum = WrapperEnum;
using EnumConstant = WrapperEnumConstant;
using Variable = WrapperVariable;
using Function = WrapperFunction;
using Parameter = WrapperParameter;
using Record = WrapperRecord;

// This defines the interface of the 'cppbind' Python module which in essence
// just consists of 'pythonized' versions of most of the classes used
// internally by CPPBind.
PYBIND11_EMBEDDED_MODULE(pycppbind, m)
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
    .def_readonly_static("TMP", &Identifier::TMP)
    .def_readonly_static("BUF", &Identifier::BUF)
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
    .def(hash(py::self))
    .def("__str__", &Identifier::str)
    .def("__bool__", [](Identifier const &Self){ return !Self.isEmpty(); })
    .def("str", &Identifier::str)
    .def("format", &Identifier::format,
         "case"_a = Identifier::ORIG_CASE,
         "quals"_a = Identifier::KEEP_QUALS)
    .def("is_empty", &Identifier::isEmpty)
    .def("components", &Identifier::components)
    .def("qualifiers", &Identifier::qualifiers)
    .def("qualified", &Identifier::qualified, "qualifiers"_a)
    .def("unqualified", &Identifier::unqualified, "remove"_a = -1);

  py::implicitly_convertible<std::string, Identifier>();

  py::class_<Wrapper, std::shared_ptr<Wrapper>>(m, "Wrapper")
    .def("includes", &Wrapper::getIncludes)
    .def("macros", &Wrapper::getMacros,
         py::return_value_policy::reference_internal)
    .def("enums", &Wrapper::getEnums,
         py::return_value_policy::reference_internal)
    .def("variables", &Wrapper::getVariables,
         py::return_value_policy::reference_internal)
    .def("functions", &Wrapper::getFunctions,
         py::return_value_policy::reference_internal)
    .def("records", &Wrapper::getRecords,
         py::return_value_policy::reference_internal);

  py::class_<Type>(m, "Type", py::dynamic_attr())
    .def(py::init<std::string>(),
         "which"_a = "void")
    .def(py::self == py::self)
    .def(py::self != py::self)
    .def(py::self < py::self)
    .def(py::self > py::self)
    .def(py::self <= py::self)
    .def(py::self >= py::self)
    .def(hash(py::self))
    .def("__str__", [](Type const &Self){ return Self.str(); })
    .def("name", &Type::getFormat,
         "with_template_postfix"_a = false)
    .def("namespace", &Type::getNamespace)
    .def("size", &Type::getSize)
    .def("is_const", &Type::isConst)
    .def("is_basic", &Type::isBasic)
    .def("is_alias", &Type::isAlias)
    .def("is_template_instantiation", &Type::isTemplateInstantiation,
         "which"_a = nullptr)
    .def("is_fundamental", &Type::isFundamental,
         "which"_a = nullptr)
    .def("is_void", &Type::isVoid)
    .def("is_boolean", &Type::isBoolean)
    .def("is_enum", &Type::isEnum)
    .def("is_scoped_enum", &Type::isScopedEnum)
    .def("is_anonymous_enum", &Type::isAnonymousEnum)
    .def("is_integral", &Type::isIntegral)
    .def("is_signed", &Type::isSigned)
    .def("is_floating", &Type::isFloating)
    .def("is_c_string", &Type::isCString)
    .def("is_reference", &Type::isReference)
    .def("is_lvalue_reference", &Type::isLValueReference)
    .def("is_rvalue_reference", &Type::isRValueReference)
    .def("is_pointer", &Type::isPointer)
    .def("is_indirection", &Type::isIndirection)
    .def("is_record", &Type::isRecord)
    .def("is_class", &Type::isClass)
    .def("is_struct", &Type::isStruct)
    .def("is_record_indirection", &Type::isRecordIndirection,
         "recursive"_a = false)
    .def("is_abstract", &Type::isAbstract)
    .def("is_polymorphic", &Type::isPolymorphic)
    .def("basic", &Type::basic)
    .def("canonical", &Type::canonical)
    .def("proxy_for", &Type::proxyFor)
    .def("base_types", &Type::baseTypes,
         "recursive"_a = false)
    .def("as_enum", &Type::asEnum)
    .def("as_record", &Type::asRecord)
    .def("template_arguments", &Type::templateArguments)
    .def("lvalue_reference_to", &Type::lvalueReferenceTo)
    .def("rvalue_reference_to", &Type::rvalueReferenceTo)
    .def("referenced", &Type::referenced)
    .def("pointer_to", &Type::pointerTo, "repeat"_a = 0u)
    .def("pointee", &Type::pointee, "recursive"_a = false)
    .def("underlying_integer_type", &Type::underlyingIntegerType)
    .def("polymorphic", &Type::polymorphic)
    .def("non_polymorphic", &Type::nonPolymorphic)
    .def("qualifiers", &Type::qualifiers)
    .def("qualified", &Type::qualified, "qualifiers"_a = 0u)
    .def("unqualified", &Type::unqualified)
    .def("with_const", &Type::withConst)
    .def("without_const", &Type::withoutConst)
    .def("str", &Type::str)
    .def("format", &Type::format,
         "with_template_postfix"_a = false,
         "with_extra_prefix"_a = "",
         "with_extra_postfix"_a = "",
         "case"_a = Identifier::ORIG_CASE,
         "quals"_a = Identifier::KEEP_QUALS)
    .def("mangled", &Type::mangled);

  py::implicitly_convertible<std::string, Type>();

  py::class_<Include>(m, "Include")
    .def(py::init<std::string, bool>(),
         "path"_a,
         "system"_a = false)
    .def("__str__", [](Include const &Self){ return Self.str(); })
    .def("str", &Include::str,
         "relative"_a = false)
    .def("path", &Include::path);

  py::class_<Macro>(m, "Macro", py::dynamic_attr())
    .def("__str__", [](Macro const &Self){ return Self.str(); })
    .def("str", &Macro::str)
    .def("name", &Macro::getName)
    .def("arg", &Macro::getArg)
    .def("as_variable", &Macro::getAsVariable,
         "type"_a = Type("int").withConst());

  py::class_<Enum>(m, "Enum", py::dynamic_attr())
    .def("name", &Enum::getName)
    .def("namespace", &Enum::getNamespace)
    .def("type", &Enum::getType)
    .def("constants", &Enum::getConstants)
    .def("is_scoped", &Enum::isScoped)
    .def("is_anonymous", &Enum::isAnonymous)
    .def("is_ambiguous", &Enum::isAmbiguous);

  py::class_<EnumConstant>(m, "EnumConstant", py::dynamic_attr())
    .def("name", &EnumConstant::getName)
    .def("namespace", &EnumConstant::getNamespace)
    .def("type", &EnumConstant::getType)
    .def("value", &EnumConstant::getValue,
         "as_c_literal"_a = false)
    .def("enum", &EnumConstant::getEnum)
    .def("as_variable", &EnumConstant::getAsVariable);

  py::class_<Variable>(m, "Variable", py::dynamic_attr())
    .def("name", &Variable::getName)
    .def("namespace", &Variable::getNamespace)
    .def("type", &Variable::getType)
    .def("getter", &Variable::getGetter)
    .def("setter", &Variable::getSetter)
    .def("is_const", &Variable::isConst)
    .def("is_constexpr", &Variable::isConstexpr)
    .def("is_assignable", &Variable::isAssignable);

  py::class_<Function>(m, "Function", py::dynamic_attr())
    .def(py::self == py::self)
    .def(py::self != py::self)
    .def("name", &Function::getFormat,
         "with_template_postfix"_a = false,
         "with_overload_postfix"_a = false,
         "without_operator_name"_a = false)
    .def("namespace", &Function::getNamespace)
    .def("origin", &Function::getOrigin)
    .def("parent", &Function::getParent)
    .def("property_for", &Function::getPropertyFor)
    .def("self", &Function::getSelf,
         py::return_value_policy::reference_internal)
    .def("parameters",
         py::overload_cast<>(&Function::getParameters, py::const_),
         py::return_value_policy::reference_internal)
    .def("return_type", &Function::getReturnType)
    .def("custom_action", &Function::getCustomAction)
    .def("overloaded_operator", &Function::getOverloadedOperator)
    .def("template_argument_list", &Function::getTemplateArgumentList)
    .def("is_definition", &Function::isDefinition)
    .def("is_member", &Function::isMember)
    .def("is_instance", &Function::isInstance)
    .def("is_constructor", &Function::isConstructor)
    .def("is_copy_constructor", &Function::isCopyConstructor)
    .def("is_copy_assignment_operator", &Function::isCopyAssignmentOperator)
    .def("is_move_constructor", &Function::isMoveConstructor)
    .def("is_move_assignment_operator", &Function::isMoveAssignmentOperator)
    .def("is_destructor", &Function::isDestructor)
    .def("is_getter", &Function::isGetter)
    .def("is_setter", &Function::isSetter)
    .def("is_static", &Function::isStatic)
    .def("is_const", &Function::isConst)
    .def("is_constexpr", &Function::isConstexpr)
    .def("is_noexcept", &Function::isNoexcept)
    .def("is_virtual", &Function::isVirtual)
    .def("is_overriding", &Function::isOverriding)
    .def("is_overloaded_operator", &Function::isOverloadedOperator,
         "which"_a = nullptr,
         "num_parameters"_a = -1)
    .def("is_base_cast", &Function::isBaseCast)
    .def("is_custom_cast", &Function::isCustomCast,
         "which"_a = nullptr)
    .def("is_template_instantiation", &Function::isTemplateInstantiation);

  py::class_<Parameter>(m, "Parameter")
    .def("name", &Parameter::getName)
    .def("namespace", &Parameter::getNamespace)
    .def("type", &Parameter::getType)
    .def("default_argument", &Parameter::getDefaultArgument)
    .def("is_self", &Parameter::isSelf);

  auto PyRecord = py::class_<Record>(m, "Record", py::dynamic_attr())
    .def(py::self == py::self)
    .def(py::self != py::self)
    .def("name", &Record::getFormat,
         "with_template_postfix"_a = false)
    .def("namespace", &Record::getNamespace)
    .def("type", &Record::getType)
    .def("bases", &Record::getBases,
         "recursive"_a = false)
    .def("functions",
         py::overload_cast<>(&Record::getFunctions, py::const_),
         py::return_value_policy::reference_internal)
    .def("constructors", &Record::getConstructors,
         py::return_value_policy::reference_internal)
    .def("default_constructor", &Record::getDefaultConstructor,
         py::return_value_policy::reference_internal)
    .def("copy_constructor", &Record::getCopyConstructor,
         py::return_value_policy::reference_internal)
    .def("copy_assignment_operator", &Record::getCopyAssignmentOperator,
         py::return_value_policy::reference_internal)
    .def("move_constructor", &Record::getMoveConstructor,
         py::return_value_policy::reference_internal)
    .def("move_assignment_operator", &Record::getMoveAssignmentOperator,
         py::return_value_policy::reference_internal)
    .def("destructor", &Record::getDestructor,
         py::return_value_policy::reference_internal)
    .def("base_cast", &Record::getBaseCast,
         "const"_a,
         py::return_value_policy::reference_internal)
    .def("template_argument_list", &Record::getTemplateArgumentList)
    .def("is_definition", &Record::isDefinition)
    .def("is_abstract", &Record::isAbstract)
    .def("is_polymorphic", &Record::isPolymorphic)
    .def("is_copyable", &Record::isCopyable)
    .def("is_movable", &Record::isMovable)
    .def("is_template_instantiation", &Record::isTemplateInstantiation);

  #define RO_PROP(NAME, VALUE) \
    .def_property_readonly_static(NAME, [](py::object const &){ return VALUE; })

  struct Options_ {};

  py::class_<Options_>(m, "Options")
    RO_PROP("output_custom_type_translation_rules_directory",
            OPT("output-custom-type-translation-rules-directory"))
    RO_PROP("output_directory", OPT("output-directory"))
    RO_PROP("output_c_header_extension", OPT("output-c-header-extension"))
    RO_PROP("output_c_source_extension", OPT("output-c-source-extension"))
    RO_PROP("output_cpp_header_extension", OPT("output-cpp-header-extension"))
    RO_PROP("output_cpp_source_extension", OPT("output-cpp-source-extension"))
    RO_PROP("output_relative_includes", OPT(bool, "output-relative-includes"))
    RO_PROP("rust_no_enums", OPT(bool, "rust-no-enums"))
    RO_PROP("lua_include_dir", OPT("lua-include-dir"))
    RO_PROP("lua_include_cpp", OPT(bool, "lua-include-cpp"));

  struct Env_ {};

  py::class_<Env_>(m, "Env")
    .def_static("get", [](std::string const &Key){ return Env().get(Key); })
    .def_static("set", [](std::string const &Key, std::string const &Val){ Env().set(Key, Val); });
}
