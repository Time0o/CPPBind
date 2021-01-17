#ifndef GUARD_WRAPPER_FUNCTION_H
#define GUARD_WRAPPER_FUNCTION_H

#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "clang/AST/APValue.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Casting.h"

#include "Identifier.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

class IdentifierIndex;

class WrapperParam
{
public:
  class DefaultArg
  {
  public:
    explicit DefaultArg(clang::Expr const *Expr);

    bool isNullptrT() const
    { return is<std::nullptr_t>(); }

    bool isInt() const
    { return is<llvm::APSInt>(); }

    bool isFloat() const
    { return is<llvm::APFloat>(); }

    bool isTrue() const
    { return BoolValue_; }

    std::string str() const;

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

  DefaultArg defaultArg() const
  {
    assert(hasDefaultArg());
    return *Default_;
  }

private:
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

  explicit WrapperFunction(clang::FunctionDecl const *Decl);

  explicit WrapperFunction(clang::CXXMethodDecl const *Decl);

  void overload(std::shared_ptr<IdentifierIndex> II);

  bool isMethod() const
  { return IsMethod_; }

  bool isConstructor() const
  { return IsConstructor_; }

  bool isDestructor() const
  { return IsDestructor_; }

  bool isOverloaded() const
  { return Overload_ > 0u; }

  WrapperType returnType() const
  { return ReturnType_; }

  Identifier name() const
  { return Name_; }

  Identifier overloadName() const
  { return OverloadName_; }

  std::vector<WrapperParam>::size_type numParams() const
  { return Params_.size(); }

  std::vector<WrapperParam>::size_type numParamsRequired() const
  {
    decltype(Params_.size()) P = 0u;

    while (P < Params_.size()) {
      if (Params_[P].hasDefaultArg())
        break;

      ++P;
    }

    return P;
  }

  std::vector<WrapperParam> params() const
  { return Params_; }

private:
  Identifier determineName(
    clang::FunctionDecl const *Decl) const;

  std::vector<WrapperParam> determineParams(
    clang::FunctionDecl const *Decl) const;

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
  {
    WrapperParam Param(std::forward<ARGS>(Args)...);

    if (!Param.hasDefaultArg()) {
#ifndef NDEBUG
      if (!Wf_.Params_.empty())
        assert(!Wf_.Params_.back().hasDefaultArg());
#endif
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
