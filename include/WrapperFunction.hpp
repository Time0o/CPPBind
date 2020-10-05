#ifndef GUARD_WRAPPER_FUNCTION_H
#define GUARD_WRAPPER_FUNCTION_H

#include <cassert>
#include <list>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "clang/AST/Decl.h"

#include "llvm/Support/Casting.h"

#include "Identifier.hpp"
#include "Options.hpp"
#include "String.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

class WrapperFunctionBuilder;

class WrapperFunction
{
  friend WrapperFunctionBuilder;

  class WrapperParam
  {
  public:
    explicit WrapperParam(WrapperType const &Type)
    : _Type(Type)
    {}

    WrapperParam(WrapperType const &Type, Identifier const &Name)
    : _Type(Type),
      _Name(Name)
    {}

    WrapperType Type() const
    { return _Type; }

    bool hasName() const
    { return _Name.has_value(); }

    Identifier Name() const
    {
      assert(_Name);
      return *_Name;
    }

    std::string strTyped() const
    { return _Type.strWrapped() + (_Name ? " " + strUntyped() : ""); }

    std::string strUntyped() const
    { return _Name ? _Name->strUnqualified("param-case") : ""; }

  private:
    WrapperType _Type;
    std::optional<Identifier> _Name;
  };

public:
  WrapperFunction(Identifier const &Name, WrapperType const &SelfType)
  : _Name(Name),
    _SelfType(SelfType)
  {}

  explicit WrapperFunction(clang::FunctionDecl const *Decl)
  : _IsMethod(false),
    _Name(determineName(Decl)),
    _Params(determineParams(Decl)),
    _ReturnType(Decl->getReturnType())
  { assert(!Decl->isTemplateInstantiation()); } // XXX

  explicit WrapperFunction(clang::CXXMethodDecl const *Decl)
  : _IsMethod(true),
    _IsConstructor(llvm::isa<clang::CXXConstructorDecl>(Decl)),
    _IsDestructor(llvm::isa<clang::CXXDestructorDecl>(Decl)),
    _IsStatic(Decl->isStatic()),
    _Name(determineName(Decl)),
    _Params(determineParams(Decl)),
    _ReturnType(Decl->getReturnType()),
    _SelfType(WrapperType(Decl->getThisType()).pointee())
  { assert(!Decl->isTemplateInstantiation()); } // XXX

  bool isMethod() const
  { return _IsMethod; }

  bool isConstructor() const
  { return _IsConstructor; }

  bool isDestructor() const
  { return _IsDestructor; }

  Identifier name() const
  { return _Name; }

  std::string strDeclaration(unsigned Overload = 0u) const
  { return strHeader(Overload) + ";"; }

  std::string strDefinition(unsigned Overload = 0u) const
  { return strHeader(Overload) + "\n" + strBody(); }

private:
  Identifier determineName(clang::FunctionDecl const *Decl) const
  { return Identifier(Decl); }

  Identifier determineName(clang::CXXMethodDecl const *Decl) const
  {
    if (_IsConstructor) {
#ifndef NDEBUG
      auto const *ConstructorDecl =
        llvm::dyn_cast<clang::CXXConstructorDecl>(Decl);
#endif

      assert(!ConstructorDecl->isCopyConstructor()); // XXX
      assert(!ConstructorDecl->isMoveConstructor()); // XXX

      return Identifier(Identifier::New).qualify(Decl->getParent());
    }

    if (_IsDestructor)
      return Identifier(Identifier::Delete).qualify(Decl->getParent());

    return Identifier(Decl);
  }

  static std::list<WrapperParam> determineParams(clang::FunctionDecl const *Decl)
  {
    std::list<WrapperParam> ParamList;

    auto Params(Decl->parameters());

    unsigned p = 0u;
    for (auto const &Param : Params) {
      auto ParamType(Param->getType());
      auto ParamName(Param->getNameAsString());

      assert(!ParamType->isReferenceType()); // XXX
      assert(!Param->hasDefaultArg()); // XXX

      if (ParamName.empty()) {
        ParamName = WRAPPER_FUNC_UNNAMED_PARAM_PLACEHOLDER;

        if (replaceAllStrs(ParamName, "%p", std::to_string(p + 1u)) == 0u)
          throw std::runtime_error("Parameter name pattern must contain at least one occurence of '%p'");
      }

      ParamList.emplace_back(ParamType, ParamName);

      ++p;
    }

    return ParamList;
  }

  static std::list<WrapperParam> determineParams(clang::CXXMethodDecl const *Decl)
  {
    assert(!Decl->isOverloadedOperator()); // XXX
    assert(!Decl->isVirtual()); // XXX

    auto ParamList(determineParams(static_cast<clang::FunctionDecl const *>(Decl)));

    if (!Decl->isStatic())
      ParamList.emplace_front(Decl->getThisType(), Identifier::Self);

    return ParamList;
  }

  bool hasParams() const
  { return !_Params.empty(); }

  std::string strParams(bool Typed, std::size_t Skip = 0u) const
  {
    std::stringstream SS;

    auto dumpParam = [&](WrapperParam const &Param)
    { return Typed ? Param.strTyped() : Param.strUntyped(); };

    SS << "(";
    if (_Params.size() > Skip) {
      auto it = _Params.begin();

      for (std::size_t i = 0u; i < Skip; ++i)
        ++it;

      SS << dumpParam(*it);
      while (++it != _Params.end())
        SS << ", " << dumpParam(*it);
    }
    SS << ")";

    return SS.str();
  };

  std::string strHeader(unsigned Overload) const
  {
    std::stringstream SS;

    SS << _ReturnType.strWrapped()
       << ' '
       << _Name.strQualified("func-case", true);

    if (Overload > 0u) {
      auto Postfix(WRAPPER_FUNC_OVERLOAD_POSTFIX);

      if (replaceAllStrs(Postfix, "%o", std::to_string(Overload)) == 0u)
        throw std::runtime_error("Overload postfix pattern must contain at least one occurence of '%o'");

      SS << Postfix;
    }

    SS << strParams(true);

    return SS.str();
  }

  std::string strBody() const
  {
    std::stringstream SS;

    SS << "{ ";

    if (_IsConstructor) {
      SS << "return " << selfCastWrapped()
         << "(new " << _SelfType.strBaseUnwrapped() << ")";
      if (hasParams())
        SS << strParams(false);
    } else if (_IsDestructor) {
      SS << "delete " << selfCastUnwrapped()
         << "(" << Identifier::Self << ")";
    } else {
      if (!_ReturnType.isVoid())
        SS << "return ";

      if (!_IsMethod || _IsStatic) {
        SS << _Name.strQualified()
           << strParams(false);
      } else {
        SS << selfCastUnwrapped()
           << "(" << Identifier::Self << ")->" << _Name.strUnqualified()
           << strParams(false, 1);
      }
    }

    SS << "; }";

    return SS.str();
  }

  std::string selfCastWrapped() const
  { return "reinterpret_cast<" + _SelfType.pointerTo().strWrapped() + ">"; }

  std::string selfCastUnwrapped() const
  { return "reinterpret_cast<" + _SelfType.pointerTo().strUnwrapped(true) + ">"; }

  bool _IsMethod = false;
  bool _IsConstructor = false;
  bool _IsDestructor = false;
  bool _IsStatic = false;

  Identifier _Name;
  std::list<WrapperParam> _Params;
  WrapperType _ReturnType;
  WrapperType _SelfType;
};

class WrapperFunctionBuilder
{
public:
  WrapperFunctionBuilder(Identifier const &Name, WrapperType const &SelfType)
  : _Wf(Name, SelfType)
  {}

  WrapperFunctionBuilder &isConstructor()
  { _Wf._IsConstructor = true; return *this; }

  WrapperFunctionBuilder &isDestructor()
  { _Wf._IsDestructor = true; return *this; }

  WrapperFunctionBuilder &isStatic()
  { _Wf._IsStatic = true; return *this; }

  template<typename TYPE>
  WrapperFunctionBuilder &setReturnType(TYPE const &Type)
  { _Wf._ReturnType = Type; return *this; }

  template<typename ...ARGS>
  WrapperFunctionBuilder &addParam(ARGS&&... Args)
  { _Wf._Params.emplace_back(std::forward<ARGS>(Args)...); return *this; }

  WrapperFunction build() const
  { return _Wf; }

private:
  WrapperFunction _Wf;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_FUNCTION_H
