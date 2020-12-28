#ifndef GUARD_WRAPPER_FUNCTION_H
#define GUARD_WRAPPER_FUNCTION_H

#include <cassert>
#include <cstddef>
#include <list>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include "clang/AST/APValue.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Casting.h"

#include "CompilerState.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Logging.hpp"
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
                              std::string const &What,
                              std::string const &CastType = "reinterpret")
  { return CastType + "_cast<" + Type + ">(" + What + ")"; }

  inline std::string typeCastWrapped(WrapperType const &Type,
                                     std::string const &What,
                                     std::shared_ptr<IdentifierIndex> II,
                                     std::string const &CastType = "reinterpret")
  { return typeCast(Type.strWrapped(II), What, CastType); }

  inline std::string typeCastUnwrapped(WrapperType const &Type,
                                       std::string const &What,
                                       std::string const &CastType = "reinterpret")
  { return typeCast(Type.strUnwrapped(true), What, CastType); }
}

class WrapperParam
{
public:
  class DefaultArg
  {
  public:
    explicit DefaultArg(clang::Expr const *Expr)
    {
      auto &Ctx(CompilerState()->getASTContext());

      if (Expr->HasSideEffects(Ctx))
        error() << "Default value must not have side effects";

      if (Expr->isValueDependent() || Expr->isTypeDependent())
        error() << "Default value must not be value/type dependent";

      if (Expr->isNullPointerConstant(Ctx, clang::Expr::NPC_NeverValueDependent)) {
        Value_ = nullptr;
        return;
      }

      clang::Expr::EvalResult Result;
      if (!Expr->EvaluateAsRValue(Result, Ctx, true))
        error() << "Default value must be constant foldable to rvalue";

      switch(Result.Val.getKind()) {
      case clang::APValue::Int:
        Value_ = Result.Val.getInt();
        break;
      case clang::APValue::Float:
        Value_ = Result.Val.getFloat();
        break;
      default:
        error() << "Default value must have pointer, integer or floating point type"; // XXX
      }

      bool ResultBool;
#if __clang_major >= 9
      if (Expr->EvaluateAsBooleanCondition(ResultBool, Ctx, true))
#else
      if (Expr->EvaluateAsBooleanCondition(ResultBool, Ctx))
#endif
        BoolValue_ = ResultBool;
    }

    std::string str() const
    {
      if (is<std::nullptr_t>())
        return "NULL";
      else if (is<llvm::APSInt>())
        return APToString(as<llvm::APSInt>());
      else if (is<llvm::APFloat>())
        return APToString(as<llvm::APFloat>());

      __builtin_unreachable();
    }

    std::string strBool() const
    { return BoolValue_ ? "true" : "false"; }

  private:
    template<typename T>
    bool is() const
    { return std::holds_alternative<T>(Value_); }

    template<typename T>
    T as() const
    { return std::get<T>(Value_); }

    template<typename T>
    static std::string APToString(T const &Val)
    {
      llvm::SmallString<40> Str;
      Val.toString(Str); // XXX format as C literal
      return Str.str().str();
    }

  private:
    std::variant<std::nullptr_t, llvm::APSInt, llvm::APFloat> Value_;
    bool BoolValue_;
  };

  WrapperParam(WrapperType const &Type,
               Identifier const &Name,
               std::optional<DefaultArg> Default = std::nullopt)
  : Type_(Type),
    Name_(Name),
    Default_(Default)
  {}

  WrapperType type() const
  { return Type_; }

  Identifier name() const
  { return Name_; }

  bool hasDefaultArg() const
  { return static_cast<bool>(Default_); }

  std::string strHeader(std::shared_ptr<IdentifierIndex> II) const
  {
    if (wrap() && Type_.isPointer())
      return detail::typedWrapped(Type_.withConst().pointerTo(), id(), II);

    return detail::typedWrapped(Type_, id(), II);
  }

  std::string strBody() const
  {
    if (Default_)
      return Type_.isFundamental("bool") ? Default_->strBool() : Default_->str();

    if (Type_.isLValueReference())
      return "*" + id();

    if (Type_.isRValueReference())
      return "std::move(" + id() + ")";

    if (wrap()) {
      return Type_.isPointer() ?
        detail::typeCastUnwrapped(Type_, id()) :
        "*" + detail::typeCastUnwrapped(Type_.withConst().pointerTo(), id());
    }

    if (!Type_.base().isCType())
      return detail::typeCastUnwrapped(Type_, id(), "static");

    return id();
  }

private:
  std::string id() const
  { return Name_.strUnqualified(PARAM_CASE); }

  bool wrap() const
  { return !Type_.base().isFundamental(); }

  WrapperType Type_;
  Identifier Name_;
  std::optional<DefaultArg> Default_;
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
    FirstDefaultParam_(determineFirstDefaultParam(Params_)),
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
    FirstDefaultParam_(determineFirstDefaultParam(Params_)),
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

  std::vector<WrapperType> types() const
  {
    std::vector<WrapperType> Types;
    for (auto const &Param : Params_)
      Types.push_back(Param.type());

    Types.push_back(ReturnType_);

    return Types;
  }

  Identifier name() const
  { return Name_; }

  std::vector<WrapperParam> params() const
  { return Params_; }

  bool hasDefaultParams() const
  { return FirstDefaultParam_ < Params_.size(); }

  void removeFirstDefaultParam()
  {
    if (!hasDefaultParams())
      return;

    auto &Param = Params_[FirstDefaultParam_];
    Param = WrapperParam(Param.type(), Param.name());

    ++FirstDefaultParam_;
  }

  void removeAllDefaultParams()
  {
    while (hasDefaultParams())
      removeFirstDefaultParam();
  }

  void overload(std::shared_ptr<IdentifierIndex> II)
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
      auto const *ParamDefaultArg(Param->getDefaultArg());

      if (ParamName.empty()) {
        ParamName = WRAPPER_FUNC_UNNAMED_PARAM_PLACEHOLDER;

        assert(replaceAllStrs(ParamName, "%p", std::to_string(i + 1u)) > 0u);
      }

      if (ParamDefaultArg)
        ParamList.emplace_back(ParamType, ParamName, WrapperParam::DefaultArg(ParamDefaultArg));
      else
        ParamList.emplace_back(ParamType, ParamName);
    }

    return ParamList;
  }

  static std::vector<WrapperParam>::size_type determineFirstDefaultParam(
    std::vector<WrapperParam> const &Params)
  {
    for (decltype(Params.size()) P = 0u; P < Params.size(); ++P) {
      if (Params[P].hasDefaultArg()) {
        for (auto P_ = P + 1u; P_ < Params.size(); ++P_)
          assert(Params[P].hasDefaultArg());

        return P;
      }
    }

    return Params.size();
  }

  bool hasParams() const
  { return !Params_.empty(); }

  std::string strParams(std::shared_ptr<IdentifierIndex> II,
                        bool Body,
                        std::vector<WrapperParam>::size_type Skip = 0u) const
  {
    std::stringstream SS;

    auto strParam = [&](WrapperParam const &Param)
    { return Body ? Param.strBody() : Param.strHeader(II); };

    SS << "(";
    if (Params_.size() > Skip) {
      for (auto P = Skip; P < Params_.size(); ++P) {
        if (!Body && Params_[P].hasDefaultArg())
          break;

        if (P > Skip)
          SS << ", ";

        SS << strParam(Params_[P]);
      }
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
    std::stringstream SS;

    SS << "{ ";

    if (IsConstructor_) {
      SS << "return " << detail::typeCastWrapped(
        self(), "new " + SelfType_.strBaseUnwrapped(), II);

      if (hasParams())
        SS << strParams(II, true);
    } else if (IsDestructor_) {
      SS << "delete " << detail::typeCastUnwrapped(self(), Identifier::Self);
    } else {
      if (!ReturnType_.isFundamental("void"))
        SS << "return ";

      if (ReturnType_.isReference())
        SS << "&";

      std::stringstream SS_;
      if (!IsMethod_ || IsStatic_) {
        SS_ << name().strQualified() << strParams(II, true);
      } else {
        SS_ << detail::typeCastUnwrapped(self(), Identifier::Self)
            << "->" << name().strUnqualified() << strParams(II, true, 1);
      }

      if (!ReturnType_.base().isCType())
        SS << detail::typeCastWrapped(ReturnType_, SS_.str(), II, "static");
      else
        SS << SS_.str();
    }

    SS << "; }";

    return SS.str();
  }

  WrapperType self() const
  { return SelfType_.pointerTo(); }

  bool IsMethod_ = false;
  bool IsConstructor_ = false;
  bool IsDestructor_ = false;
  bool IsStatic_ = false;
  unsigned Overload_ = 0u;

  Identifier Name_;
  Identifier OverloadName_;

  std::vector<WrapperParam> Params_;
  std::vector<WrapperParam>::size_type FirstDefaultParam_;
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
  {
    WrapperParam Param(std::forward<ARGS>(Args)...);

    if (!Param.hasDefaultArg()) {
#ifndef NDEBUG
      if (!Wf_.Params_.empty())
        assert(!Wf_.Params_.back().hasDefaultArg());
#endif

      Wf_.FirstDefaultParam_ = Wf_.Params_.size() + 1u;
    }

    Wf_.Params_.emplace_back(Param);

    return *this;
  }

  WrapperFunction build() const
  { return Wf_; }

private:
  WrapperFunction Wf_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_FUNCTION_H
