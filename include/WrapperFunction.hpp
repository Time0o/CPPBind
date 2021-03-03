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
#include "LLVMFormat.hpp"
#include "TemplateArgument.hpp"
#include "WrapperObject.hpp"
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

class WrapperParameter : public WrapperObject<clang::ParmVarDecl>
{
public:
  WrapperParameter(Identifier const &Name, WrapperType const &Type);

  WrapperParameter(Identifier const &Name, clang::ParmVarDecl const *Decl);

  static WrapperParameter self(WrapperType const &Type)
  { return WrapperParameter(Identifier(Identifier::SELF), Type); }

  Identifier getName() const
  { return Name_; }

  WrapperType getType() const
  { return Type_; }

  std::optional<std::string> getDefaultArgument() const
  { return DefaultArgument_; }

  bool isSelf() const
  { return Name_ == Identifier(Identifier::SELF); }

private:
  static std::optional<std::string> determineDefaultArgument(
    clang::ParmVarDecl const *Decl);

  Identifier Name_;
  WrapperType Type_;

  std::optional<std::string> DefaultArgument_;
};

class WrapperFunction : public WrapperObject<clang::FunctionDecl>
{
  friend class WrapperFunctionBuilder;

  struct OverloadedOperator { std::string Name; std::string Spelling; };

public:
  WrapperFunction(Identifier const &Name);

  explicit WrapperFunction(clang::FunctionDecl const *Decl);

  explicit WrapperFunction(clang::CXXMethodDecl const *Decl);

  bool operator==(WrapperFunction const &Other) const;

  bool operator!=(WrapperFunction const &Other) const
  { return !(this->operator==(Other)); }

  void overload(std::shared_ptr<IdentifierIndex> II);

  Identifier getName(bool WithTemplatePostfix = false,
                     bool WithoutOperatorName = false,
                     bool Overloaded = false) const;

  std::vector<WrapperParameter const *> getParameters(bool SkipSelf = false) const;

  WrapperType getReturnType() const
  { return ReturnType_; }

  std::optional<std::string> getOverloadedOperator() const;

  std::optional<std::string> getTemplateArgumentList() const;

  bool isDefinition() const
  { return IsDefinition_; }

  bool isMember() const
  { return IsMember_; }

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
  { return static_cast<bool>(TemplateArgumentList_); }

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

  static std::optional<TemplateArgumentList>
  determineTemplateArgumentList(clang::FunctionDecl const *Decl);

  std::optional<TemplateArgumentList> TemplateArgumentList_;

  std::optional<OverloadedOperator> OverloadedOperator_;

  std::optional<unsigned> Overload_;

  Identifier Name_;
  WrapperType ReturnType_;
  std::deque<WrapperParameter> Parameters_;

  bool IsDefinition_ = false;
  bool IsMember_ = false;
  bool IsConstructor_ = false;
  bool IsDestructor_ = false;
  bool IsStatic_ = false;
  bool IsConst_ = false;
  bool IsNoexcept_ = false;
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

namespace llvm
{
  LLVM_FORMAT_PROVIDER(cppbind::WrapperParameter);
  LLVM_FORMAT_PROVIDER(cppbind::WrapperFunction);
}

#endif // GUARD_WRAPPER_FUNCTION_H
