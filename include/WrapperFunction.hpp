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

namespace detail
{
  // XXX move this into WrapperType?

  inline std::string typed(std::string const &Type,
                           std::string const &What)
  { return Type + " " + What; }

  inline std::string typedWrapped(WrapperType const &Type,
                                  std::string const &What,
                                  std::shared_ptr<IdentifierIndex> II)
  { return Type.strWrapped(II) + " " + What; }

  inline std::string typedUnwrapped(WrapperType const &Type,
                                    std::string const &What)
  { return Type.strUnwrapped(true) + " " + What; }

  inline std::string typeCast(std::string const &Type,
                              std::string const &What)
  { return "reinterpret_cast<" + Type + ">" + "(" + What + ")"; }

  inline std::string typeCastWrapped(WrapperType const &Type,
                                     std::string const &What,
                                     std::shared_ptr<IdentifierIndex> II)
  { return typeCast(Type.strWrapped(II), What); }

  inline std::string typeCastUnwrapped(WrapperType const &Type,
                                       std::string const &What)
  { return typeCast(Type.strUnwrapped(true), What); }

  inline std::string dereference(std::string const &What)
  { return "*" + What; }
}

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

  std::string strHeader(std::shared_ptr<IdentifierIndex> II) const
  {
    if (wrap()) {
      return Type_.isPointer() ?
        detail::typedWrapped(Type_, id(), II) :
        detail::typedWrapped(Type_.withConst().pointerTo(), id(), II);
    }

    return detail::typedUnwrapped(Type_, id());
  }

  std::string strBody() const
  {
    if (wrap()) {
      return Type_.isPointer() ?
        detail::typeCastUnwrapped(Type_, id()) :
        detail::dereference(detail::typeCastUnwrapped(Type_.withConst().pointerTo(), id()));
    }

    return id();
  }

private:
  std::string id() const
  { return Name_.strUnqualified(PARAM_CASE); }

  bool wrap() const
  { return !Type_.base().isFundamental(); }

  WrapperType Type_;
  Identifier Name_;
};

class WrapperFunctionBuilder;

class WrapperFunction
{
  friend WrapperFunctionBuilder;

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
    Params_(determineParams(Decl, IsMethod_)),
    ReturnType_(Decl->getReturnType())
  { assert(!Decl->isTemplateInstantiation()); } // XXX

  explicit WrapperFunction(clang::CXXMethodDecl const *Decl)
  : IsMethod_(true),
    IsConstructor_(llvm::isa<clang::CXXConstructorDecl>(Decl)),
    IsDestructor_(llvm::isa<clang::CXXDestructorDecl>(Decl)),
    IsStatic_(Decl->isStatic()),
    Name_(determineName(Decl)),
    OverloadName_(Name_),
    Params_(determineParams(Decl, IsMethod_)),
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

  std::vector<WrapperParam> params() const
  { return Params_; }

  void resolveOverload(std::shared_ptr<IdentifierIndex> II)
  {
    if (Overload_ == 0u) {
      if ((Overload_ = II->popOverload(name())) == 0u)
        return;
    }

    auto Postfix(WRAPPER_FUNC_OVERLOAD_POSTFIX);

    assert(replaceAllStrs(Postfix, "%o", std::to_string(Overload_)) > 0u);

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

  static std::vector<WrapperParam> determineParams(
    clang::FunctionDecl const *Decl,
    bool IsMethod)
  {
    std::vector<WrapperParam> ParamList;

    if (IsMethod) {
      auto const *MethodDecl = dynamic_cast<clang::CXXMethodDecl const *>(Decl);

      assert(!MethodDecl->isOverloadedOperator()); // XXX
      assert(!MethodDecl->isVirtual()); // XXX

      if (!MethodDecl->isStatic())
        ParamList.emplace_back(MethodDecl->getThisType(), Identifier::Self);
    }

    auto Params(Decl->parameters());

    for (unsigned i = 0u; i < Params.size(); ++i) {
      auto const &Param(Params[i]);
      auto ParamType(Param->getType());
      auto ParamName(Param->getNameAsString());

      assert(!ParamType->isReferenceType()); // XXX
      assert(!Param->hasDefaultArg()); // XXX

      if (ParamName.empty()) {
        ParamName = WRAPPER_FUNC_UNNAMED_PARAM_PLACEHOLDER;

        assert(replaceAllStrs(ParamName, "%p", std::to_string(i + 1u)) > 0u);
      }

      ParamList.emplace_back(ParamType, ParamName);
    }

    return ParamList;
  }

  bool hasParams() const
  { return !Params_.empty(); }

  std::string strParams(std::shared_ptr<IdentifierIndex> II,
                        bool Body,
                        std::size_t Skip = 0u) const
  {
    std::stringstream SS;

    auto strParam = [&](WrapperParam const &Param)
    { return Body ? Param.strBody() : Param.strHeader(II); };

    SS << "(";
    if (Params_.size() > Skip) {
      auto it = Params_.begin();

      for (std::size_t i = 0u; i < Skip; ++i)
        ++it;

      SS << strParam(*it);
      while (++it != Params_.end())
        SS << ", " << strParam(*it);
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

    SS << strParams(II, false);

    return SS.str();
  }

  std::string strBody(std::shared_ptr<IdentifierIndex> II) const
  {
    auto SelfTypePtr(SelfType_.pointerTo());

    std::stringstream SS;

    SS << "{ ";

    if (IsConstructor_) {
      SS << "return " << detail::typeCastWrapped(
        SelfTypePtr, "new " + SelfType_.strBaseUnwrapped(), II);

      if (hasParams())
        SS << strParams(II, true);
    } else if (IsDestructor_) {
      SS << "delete " << detail::typeCastUnwrapped(SelfTypePtr, Identifier::Self);
    } else {
      if (!ReturnType_.isFundamental("void"))
        SS << "return ";

      if (!IsMethod_ || IsStatic_) {
        SS << name().strQualified() << strParams(II, true);
      } else {
        SS << detail::typeCastUnwrapped(SelfTypePtr, Identifier::Self)
           << "->" << name().strUnqualified() << strParams(II, true, 1);
      }
    }

    SS << "; }";

    return SS.str();
  }

  bool IsMethod_ = false;
  bool IsConstructor_ = false;
  bool IsDestructor_ = false;
  bool IsStatic_ = false;
  unsigned Overload_ = 0u;

  Identifier Name_;
  Identifier OverloadName_;

  std::vector<WrapperParam> Params_;
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
