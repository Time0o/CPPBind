#ifndef GUARD_WRAPPER_FUNCTION_H
#define GUARD_WRAPPER_FUNCTION_H

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "llvm/ADT/SmallString.h"

#include "Identifier.hpp"
#include "WrapperType.hpp"

namespace clang
{
  class CXXMethodDecl;
  class Expr;
  class FunctionDecl;
}

namespace llvm
{
  class APSInt;
  class APFloat;
}

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
  WrapperFunction(Identifier const &Name,
                  WrapperType const &SelfType = WrapperType())
  : Name_(Name),
    NameOverloaded_(Name),
    SelfType_(SelfType)
  {}

  explicit WrapperFunction(clang::FunctionDecl const *Decl);

  explicit WrapperFunction(clang::CXXMethodDecl const *Decl);

  void overload(std::shared_ptr<IdentifierIndex> II);

  bool isMember() const
  { return !SelfType_.isVoid(); }

  bool isConstructor() const
  { return IsConstructor_; }

  bool isDestructor() const
  { return IsDestructor_; }

  bool isOverloaded() const
  { return Overload_ > 0u; }

  WrapperType selfType() const
  { return SelfType_; }

  WrapperType returnType() const
  { return ReturnType_; }

  Identifier name() const
  { return Name_; }

  Identifier nameOverloaded() const
  { return NameOverloaded_; }

  std::vector<WrapperParam> parameters(bool RequiredOnly = false) const
  {
    if (!RequiredOnly)
      return Params_;

    std::vector<WrapperParam> RequiredParams;
    RequiredParams.reserve(Params_.size());

    for (auto const &Param : Params_) {
      if (Param.hasDefaultArg())
        break;

      RequiredParams.push_back(Param);
    }

    return RequiredParams;
  }

private:
  Identifier determineName(
    clang::FunctionDecl const *Decl) const;

  std::vector<WrapperParam> determineParams(
    clang::FunctionDecl const *Decl) const;

  bool IsConstructor_ = false;
  bool IsDestructor_ = false;
  bool IsStatic_ = false;

  unsigned Overload_ = 0u;

  Identifier Name_;
  Identifier NameOverloaded_;

  WrapperType SelfType_;
  WrapperType ReturnType_;
  std::vector<WrapperParam> Params_;
};

class WrapperFunctionBuilder
{
public:
  WrapperFunctionBuilder(Identifier const &Name,
                         WrapperType const &SelfType = WrapperType())
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
