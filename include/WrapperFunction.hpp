#ifndef GUARD_WRAPPER_FUNCTION_H
#define GUARD_WRAPPER_FUNCTION_H

#include <cassert>
#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/TemplateBase.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"

#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
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
class WrapperRecord;

class WrapperParameter
{
  class DefaultArgument
  {
  public:
    explicit DefaultArgument(clang::Expr const *Expr);

    bool isNullptrT() const
    { return is<std::nullptr_t>(); }

    bool isInt() const
    { return is<llvm::APSInt>(); }

    bool isFloat() const
    { return is<llvm::APFloat>(); }

    std::string str() const;

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
  };

public:
  WrapperParameter(Identifier const &Name,
                   WrapperType const &Type,
                   clang::Expr const *DefaultExpr = nullptr)
  : Name_(Name),
    Type_(Type)
  {
    if (DefaultExpr)
      DefaultArgument_ = DefaultArgument(DefaultExpr);
  }

  WrapperParameter(clang::ParmVarDecl const *Decl)
  : WrapperParameter(Identifier(Decl->getNameAsString()),
                     WrapperType(Decl->getType()),
                     Decl->getDefaultArg())
  {}

  static WrapperParameter self(WrapperType const &Type)
  { return WrapperParameter(Identifier(Identifier::SELF), Type); }

  Identifier getName() const
  { return Name_; }

  WrapperType getType() const
  { return Type_; }

  std::optional<std::string> getDefaultArgument() const
  {
    if (!DefaultArgument_)
      return std::nullopt;

    return DefaultArgument_->str();
  }

  bool isSelf() const
  { return Name_ == Identifier(Identifier::SELF); }

private:
  Identifier Name_;
  WrapperType Type_;

  std::optional<DefaultArgument> DefaultArgument_;
};

class WrapperFunction
{
  friend class WrapperFunctionBuilder;

  struct OverloadedOperator
  {
    OverloadedOperator(std::string const &Name, std::string const &Spelling)
    : Name(Name),
      Spelling(Spelling)
    {}

    std::string Name;
    std::string Spelling;
  };

  class TemplateArgument
  {
  public:
    explicit TemplateArgument(clang::TemplateArgument const &Arg)
    : Arg_(Arg)
    {}

    bool operator==(TemplateArgument const &Other) const
    { return Arg_.structurallyEquals(Other.Arg_); }

    bool operator!=(TemplateArgument const &Other) const
    { return !operator==(Other); }

    std::string str() const
    {
      std::string Str;
      llvm::raw_string_ostream SS(Str);

      Arg_.dump(SS);

      return SS.str();
    }

  private:
    clang::TemplateArgument Arg_;
  };

public:
  WrapperFunction(Identifier const &Name)
  : Name_(Name),
    ReturnType_("void")
  {}

  explicit WrapperFunction(clang::FunctionDecl const *Decl);

  explicit WrapperFunction(clang::CXXMethodDecl const *Decl);

  bool operator==(WrapperFunction const &Other) const;

  bool operator!=(WrapperFunction const &Other) const
  { return !(this->operator==(Other)); }

  void overload(std::shared_ptr<IdentifierIndex> II);

  Identifier getName(bool Overloaded = false,
                     bool WithoutOperatorName = false,
                     bool WithTemplatePostfix = false) const;

  WrapperRecord const *getParent() const
  { return Parent_; }

  std::vector<WrapperParameter const *> getParameters(bool SkipSelf = false) const;

  WrapperType getReturnType() const
  { return ReturnType_; }

  std::optional<std::string> getOverloadedOperator() const;

  std::optional<std::string> getTemplateArgumentList() const;

  bool isDefinition() const
  { return IsDefinition_; }

  bool isMember() const
  { return Parent_; }

  bool isInstance() const
  { return isMember() && !isConstructor() && !isStatic(); }

  bool isConstructor() const
  { return IsConstructor_; }

  bool isDestructor() const
  { return IsDestructor_; }

  bool isStatic() const
  { return IsStatic_; }

  bool isConst() const
  { return IsConst_; }

  bool isNoexcept() const
  { return IsNoexcept_; }

  bool isOverloaded() const
  { return static_cast<bool>(Overload_); }

  bool isOverloadedOperator() const
  { return static_cast<bool>(OverloadedOperator_); }

  bool isTemplateInstantiation() const
  { return static_cast<bool>(TemplateArguments_); }

private:
  static Identifier
  determineName(clang::FunctionDecl const *Decl);

  static WrapperType
  determineReturnType(clang::FunctionDecl const *Decl);

  static std::deque<WrapperParameter>
  determineParameters(clang::FunctionDecl const *Decl);

  static void
  postfixParameterName(std::string &ParamName, unsigned p);

  static bool
  determineIfNoexcept(clang::FunctionDecl const *Decl);

  static std::optional<OverloadedOperator>
  determineOverloadedOperator(clang::FunctionDecl const *Decl);

  static std::optional<std::deque<TemplateArgument>>
  determineTemplateArguments(clang::FunctionDecl const *Decl);

  WrapperRecord const *Parent_ = nullptr;

  Identifier Name_;
  WrapperType ReturnType_;
  std::deque<WrapperParameter> Parameters_;

  bool IsDefinition_ = false;
  bool IsConstructor_ = false;
  bool IsDestructor_ = false;
  bool IsStatic_ = false;
  bool IsConst_ = false;
  bool IsNoexcept_ = false;

  std::optional<unsigned> Overload_;
  std::optional<OverloadedOperator> OverloadedOperator_;
  std::optional<std::deque<TemplateArgument>> TemplateArguments_;
};

class WrapperFunctionBuilder
{
public:
  template<typename ...ARGS>
  WrapperFunctionBuilder(ARGS &&...args)
  : Wf_(std::forward<ARGS>(args)...)
  {}

  WrapperFunctionBuilder &setName(Identifier const &Name);
  WrapperFunctionBuilder &setParent(WrapperRecord const *Parent);
  WrapperFunctionBuilder &setReturnType(WrapperType const &ReturnType);
  WrapperFunctionBuilder &addParameter(WrapperParameter const &Param);
  WrapperFunctionBuilder &setIsConstructor(bool Val = true);
  WrapperFunctionBuilder &setIsDestructor(bool Val = true);
  WrapperFunctionBuilder &setIsStatic(bool Val = true);
  WrapperFunctionBuilder &setIsConst(bool Val = true);
  WrapperFunctionBuilder &setIsNoexcept(bool Val = true);

  WrapperFunction build() const
  { return Wf_; }

private:
  WrapperFunction Wf_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_FUNCTION_H
