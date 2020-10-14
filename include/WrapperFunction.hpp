#ifndef GUARD_WRAPPER_FUNCTION_H
#define GUARD_WRAPPER_FUNCTION_H

#include <cassert>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "clang/AST/Decl.h"

#include "llvm/Support/Casting.h"

#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
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
    WrapperParam(WrapperType const &Type, Identifier const &Name)
    : Type_(Type),
      Name_(Name)
    {}

    WrapperType type() const
    { return Type_; }

    Identifier name() const
    { return Name_; }

    std::string strTyped(std::shared_ptr<IdentifierIndex> II) const
    { return Type_.strWrapped(II) + " " + strUntyped(); }

    std::string strUntyped() const
    { return Name_.strUnqualified(PARAM_CASE); }

  private:
    WrapperType Type_;
    Identifier Name_;
  };

public:
  WrapperFunction(Identifier const &Name, WrapperType const &SelfType)
  : Name_(Name),
    OverloadName_(Name),
    SelfType_(SelfType)
  {}

  explicit WrapperFunction(clang::FunctionDecl const *Decl)
  : IsMethod_(false),
    Name_(determineName(Decl)),
    OverloadName_(Name_),
    Params_(determineParams(Decl)),
    ReturnType_(Decl->getReturnType())
  { assert(!Decl->isTemplateInstantiation()); } // XXX

  explicit WrapperFunction(clang::CXXMethodDecl const *Decl)
  : IsMethod_(true),
    IsConstructor_(llvm::isa<clang::CXXConstructorDecl>(Decl)),
    IsDestructor_(llvm::isa<clang::CXXDestructorDecl>(Decl)),
    IsStatic_(Decl->isStatic()),
    Name_(determineName(Decl)),
    OverloadName_(Name_),
    Params_(determineParams(Decl)),
    ReturnType_(Decl->getReturnType()),
    SelfType_(WrapperType(Decl->getThisType()).pointee())
  { assert(!Decl->isTemplateInstantiation()); } // XXX

  bool isMethod() const
  { return IsMethod_; }

  bool isConstructor() const
  { return IsConstructor_; }

  bool isDestructor() const
  { return IsDestructor_; }

  bool isOverloaded() const
  { return Overload_ > 0u; }

  Identifier name() const
  { return Name_; }

  void resolveOverload(std::shared_ptr<IdentifierIndex> II)
  {
    if (Overload_ == 0u) {
      if ((Overload_ = II->popOverload(name())) == 0u)
        return;
    }

    auto Postfix(WRAPPER_FUNC_OVERLOAD_POSTFIX);

    if (replaceAllStrs(Postfix, "%o", std::to_string(Overload_)) == 0u)
      throw std::runtime_error("Overload postfix pattern must contain at least one occurence of '%o'");

    OverloadName_ = name() + Postfix;

    II->add(OverloadName_, IdentifierIndex::FUNC);
  }

  std::string strDeclaration(std::shared_ptr<IdentifierIndex> II) const
  { return strHeader(II) + ";"; }

  std::string strDefinition(std::shared_ptr<IdentifierIndex> II) const
  { return strHeader(II) + "\n" + strBody(II); }

private:
  Identifier determineName(clang::FunctionDecl const *Decl) const
  { return Identifier(Decl); }

  Identifier determineName(clang::CXXMethodDecl const *Decl) const
  {
    if (IsConstructor_) {
#ifndef NDEBUG
      auto const *ConstructorDecl =
        llvm::dyn_cast<clang::CXXConstructorDecl>(Decl);
#endif

      assert(!ConstructorDecl->isCopyConstructor()); // XXX
      assert(!ConstructorDecl->isMoveConstructor()); // XXX

      return Identifier(Identifier::New).qualify(Decl->getParent());
    }

    if (IsDestructor_)
      return Identifier(Identifier::Delete).qualify(Decl->getParent());

    return Identifier(Decl);
  }

  static std::list<WrapperParam> determineParams(clang::FunctionDecl const *Decl)
  {
    std::list<WrapperParam> ParamList;

    auto Params(Decl->parameters());

    for (unsigned i = 0u; i < Params.size(); ++i) {
      auto const &Param(Params[i]);
      auto ParamType(Param->getType());
      auto ParamName(Param->getNameAsString());

      assert(!ParamType->isReferenceType()); // XXX
      assert(!Param->hasDefaultArg()); // XXX

      if (ParamName.empty()) {
        ParamName = WRAPPER_FUNC_UNNAMED_PARAM_PLACEHOLDER;

        if (replaceAllStrs(ParamName, "%p", std::to_string(i + 1u)) == 0u)
          throw std::runtime_error("Parameter name pattern must contain at least one occurence of '%p'");
      }

      ParamList.emplace_back(ParamType, ParamName);
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
  { return !Params_.empty(); }

  std::string strParams(std::shared_ptr<IdentifierIndex> II,
                        bool Typed,
                        std::size_t Skip = 0u) const
  {
    std::stringstream SS;

    auto dumpParam = [&](WrapperParam const &Param)
    { return Typed ? Param.strTyped(II) : Param.strUntyped(); };

    SS << "(";
    if (Params_.size() > Skip) {
      auto it = Params_.begin();

      for (std::size_t i = 0u; i < Skip; ++i)
        ++it;

      SS << dumpParam(*it);
      while (++it != Params_.end())
        SS << ", " << dumpParam(*it);
    }
    SS << ")";

    return SS.str();
  };

  std::string strHeader(std::shared_ptr<IdentifierIndex> II) const
  {
    std::stringstream SS;

    auto Name = Overload_ > 0u ? OverloadName_ : name();

    SS << ReturnType_.strWrapped(II)
       << ' '
       << II->alias(Name).strQualified(FUNC_CASE, true);

    SS << strParams(II, true);

    return SS.str();
  }

  std::string strBody(std::shared_ptr<IdentifierIndex> II) const
  {
    std::stringstream SS;

    SS << "{ ";

    if (IsConstructor_) {
      SS << "return " << selfCastWrapped(II)
         << "(new " << SelfType_.strBaseUnwrapped() << ")";
      if (hasParams())
        SS << strParams(II, false);
    } else if (IsDestructor_) {
      SS << "delete " << selfCastUnwrapped()
         << "(" << Identifier::Self << ")";
    } else {
      if (!ReturnType_.isFundamental("void"))
        SS << "return ";

      if (!IsMethod_ || IsStatic_) {
        SS << name().strQualified() << strParams(II, false);
      } else {
        SS << selfCastUnwrapped()
           << "(" << Identifier::Self << ")->" << name().strUnqualified()
           << strParams(II, false, 1);
      }
    }

    SS << "; }";

    return SS.str();
  }

  std::string selfCastWrapped(std::shared_ptr<IdentifierIndex> II) const
  { return "reinterpret_cast<" + SelfType_.pointerTo().strWrapped(II) + ">"; }

  std::string selfCastUnwrapped() const
  { return "reinterpret_cast<" + SelfType_.pointerTo().strUnwrapped(true) + ">"; }

  bool IsMethod_ = false;
  bool IsConstructor_ = false;
  bool IsDestructor_ = false;
  bool IsStatic_ = false;
  unsigned Overload_ = 0u;

  Identifier Name_;
  Identifier OverloadName_;

  std::list<WrapperParam> Params_;
  WrapperType ReturnType_;
  WrapperType SelfType_;
};

class WrapperFunctionBuilder
{
public:
  WrapperFunctionBuilder(Identifier const &Name, WrapperType const &SelfType)
  : Wf_(Name, SelfType)
  {}

  WrapperFunctionBuilder &isConstructor()
  { Wf_.IsConstructor_ = true; return *this; }

  WrapperFunctionBuilder &isDestructor()
  { Wf_.IsDestructor_ = true; return *this; }

  WrapperFunctionBuilder &isStatic()
  { Wf_.IsStatic_ = true; return *this; }

  template<typename TYPE>
  WrapperFunctionBuilder &setReturnType(TYPE const &Type)
  { Wf_.ReturnType_ = Type; return *this; }

  template<typename ...ARGS>
  WrapperFunctionBuilder &addParam(ARGS&&... Args)
  { Wf_.Params_.emplace_back(std::forward<ARGS>(Args)...); return *this; }

  WrapperFunction build() const
  { return Wf_; }

private:
  WrapperFunction Wf_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_FUNCTION_H
