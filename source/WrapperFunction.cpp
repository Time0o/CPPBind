#include "WrapperFunction.hpp"

#include <cassert>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "clang/AST/APValue.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclarationName.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Type.h"
#include "clang/Frontend/CompilerInstance.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include "ClangUtils.hpp"
#include "CompilerState.hpp"
#include "Error.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "String.hpp"
#include "Util.hpp"
#include "WrapperRecord.hpp"

namespace cppbind
{

WrapperDefaultArgument::WrapperDefaultArgument(clang::Expr const *Expr)
{
  auto &Ctx(ASTContext());

  if (Expr->HasSideEffects(Ctx))
    throw CPPBindError("default value must not have side effects");

  if (Expr->isValueDependent() || Expr->isTypeDependent())
    throw CPPBindError("default value must not be value/type dependent");

  if (Expr->isNullPointerConstant(Ctx, clang::Expr::NPC_NeverValueDependent)) {
    Value_ = nullptr;
    return;
  }

  clang::Expr::EvalResult Result;
  if (!Expr->EvaluateAsRValue(Result, Ctx, true))
    throw CPPBindError("default value must be constant foldable to rvalue");

  switch(Result.Val.getKind()) {
  case clang::APValue::Int:
    Value_ = Result.Val.getInt();
    break;
  case clang::APValue::Float:
    Value_ = Result.Val.getFloat();
    break;
  default:
    throw CPPBindError("default value must have pointer, integer or floating point type"); // XXX
  }

  bool ResultBool;
#if __clang_major >= 9
  if (Expr->EvaluateAsBooleanCondition(ResultBool, Ctx, true))
#else
  if (Expr->EvaluateAsBooleanCondition(ResultBool, Ctx))
#endif
    BoolValue_ = ResultBool;
}

std::string WrapperDefaultArgument::str() const
{
  if (isNullptrT())
    return "nullptr";
  else if (isInt())
    return APToString(as<llvm::APSInt>());
  else if (isFloat())
    return APToString(as<llvm::APFloat>());

  llvm_unreachable("invalid default argument type");
}

WrapperFunction::WrapperFunction(clang::FunctionDecl const *Decl)
: Name_(determineName(Decl)),
  ReturnType_(determineReturnType(Decl)),
  Parameters_(determineParameters(Decl)),
  IsDefinition_(Decl->isThisDeclarationADefinition()),
  IsNoexcept_(determineIfNoexcept(Decl)),
  OverloadedOperator_(determineOverloadedOperator(Decl))
{ assert(!Decl->isTemplateInstantiation()); } // XXX

WrapperFunction::WrapperFunction(clang::CXXMethodDecl const *Decl)
: Name_(determineName(Decl)),
  ReturnType_(determineReturnType(Decl)),
  Parameters_(determineParameters(Decl)),
  IsDefinition_(Decl->isThisDeclarationADefinition()),
  IsConstructor_(decl::isConstructor(Decl)),
  IsDestructor_(decl::isDestructor(Decl)),
  IsStatic_(Decl->isStatic()),
  IsConst_(Decl->isConst()),
  IsNoexcept_(determineIfNoexcept(Decl)),
  OverloadedOperator_(determineOverloadedOperator(Decl))
{
  assert(!Decl->isTemplateInstantiation()); // XXX
}

bool
WrapperFunction::operator==(WrapperFunction const &Other) const
{
  if (Name_ != Other.Name_)
    return false;

  if (Parent_ != Other.Parent_)
    return false;

  if (ReturnType_ != Other.ReturnType_)
    return false;

  if (Parameters_.size() != Other.Parameters_.size())
    return false;

  for (std::size_t i = 0; i < Parameters_.size(); ++i) {
    if (Parameters_[i].getType() != Other.Parameters_[i].getType())
      return false;
  }

  return true;
}

void
WrapperFunction::overload(std::shared_ptr<IdentifierIndex> II)
{
  if (!II->hasOverload(Name_))
    return;

  Overload_ = II->popOverload(Name_);
}

Identifier
WrapperFunction::getName(bool Overloaded, bool ReplaceOperatorName) const
{
  Identifier Name(Name_);

  if (ReplaceOperatorName && isOverloadedOperator()) {
    auto NameStr(Name.str());

    string::replace(NameStr,
                    OverloadedOperator_->Spelling,
                    OverloadedOperator_->Name);

    Name = Identifier(NameStr);
  }

  if (Overloaded && isOverloaded()) {
    auto Postfix = OPT("wrapper-func-overload-postfix");

    string::replaceAll(Postfix, "%o", std::to_string(*Overload_));

    Name = Identifier(Name.str() + Postfix);
  }

  return Name;
}

std::vector<WrapperParameter const *>
WrapperFunction::getParameters(bool SkipSelf) const
{
  if (SkipSelf && !Parameters_.empty() && Parameters_.front().isSelf())
    return util::vectorOfPointers(Parameters_.begin() + 1, Parameters_.end());

  return util::vectorOfPointers(Parameters_.begin(), Parameters_.end());
}

std::optional<std::string>
WrapperFunction::getOverloadedOperator() const
{
  if (!OverloadedOperator_)
    return std::nullopt;

  return OverloadedOperator_->Name;
}

Identifier
WrapperFunction::determineName(clang::FunctionDecl const *Decl)
{
  if (decl::isConstructor(Decl))
    return Identifier(Identifier::NEW);

  if (decl::isDestructor(Decl))
    return Identifier(Identifier::DELETE);

  return Identifier(Decl);
}

WrapperType
WrapperFunction::determineReturnType(clang::FunctionDecl const *Decl)
{ return WrapperType(Decl->getReturnType()); }

std::deque<WrapperParameter>
WrapperFunction::determineParameters(clang::FunctionDecl const *Decl)
{
  std::deque<WrapperParameter> ParamList;

  auto Params(Decl->parameters());

  for (unsigned i = 0u; i < Params.size(); ++i) {
    auto const &Param(Params[i]);

    auto ParamName(Param->getNameAsString());

    if (ParamName.empty()) {
      ParamName = OPT("wrapper-func-unnamed-param-placeholder");

      string::replaceAll(ParamName, "%p", std::to_string(i + 1u));

      ParamList.emplace_back(Identifier(ParamName),
                             WrapperType(Param->getType()),
                             Param->getDefaultArg());

    } else {

      ParamList.emplace_back(Param);
    }
  }

  return ParamList;
}

bool
WrapperFunction::determineIfNoexcept(clang::FunctionDecl const *Decl)
{
# if __clang_major__ >= 9
  auto EST = Decl->getExceptionSpecType();
#else
  auto const *ProtoType = Decl->getType()->getAs<clang::FunctionProtoType>();
  auto EST = ProtoType->getExceptionSpecType();
#endif

  return EST == clang::EST_BasicNoexcept || EST == clang::EST_NoexceptTrue;
}

std::optional<WrapperFunction::OverloadedOperator>
WrapperFunction::determineOverloadedOperator(clang::FunctionDecl const *Decl)
{
  using namespace clang;

  if (!Decl->isOverloadedOperator())
    return std::nullopt;

  switch (Decl->getOverloadedOperator()) {
#define OVERLOADED_OPERATOR(Name,Spelling,Token,Unary,Binary,MemberOnly) \
  case OO_##Name: return OverloadedOperator(#Name, Spelling);
#include "clang/Basic/OperatorKinds.def"
  default:
    llvm_unreachable("invalid overloaded operator kind");
  }
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setParent(WrapperRecord const *Parent)
{
  auto ParentName(Parent->getName());
  auto ParentType(Parent->getType());

  Wf_.Parent_ = Parent;

  Wf_.Name_ = Wf_.Name_.unqualified().qualified(ParentName);

  if (Wf_.isInstance()) {
    WrapperParameter Self(Identifier(Identifier::SELF), ParentType.pointerTo());

    if (!Wf_.Parameters_.empty() && Wf_.Parameters_.front().isSelf())
      Wf_.Parameters_.front() = Self;
    else
      Wf_.Parameters_.insert(Wf_.Parameters_.begin(), Self);

  } else if (Wf_.isConstructor()) {
    Wf_.ReturnType_ = ParentType.pointerTo();
  }

  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setName(Identifier const &Name)
{
  Wf_.Name_ = Name;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setReturnType(WrapperType const &ReturnType)
{
  Wf_.ReturnType_ = ReturnType;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::addParameter(WrapperParameter const &Param)
{
#ifndef NDEBUG
  if (!Param.getDefaultArgument() && !Wf_.Parameters_.empty())
      assert(!Wf_.Parameters_.back().getDefaultArgument());
#endif

  Wf_.Parameters_.emplace_back(Param);
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsConstructor(bool Val)
{
  //assert(Wf_.isMember()); // XXX
  Wf_.IsConstructor_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsDestructor(bool Val)
{
  //assert(Wf_.isMember()); // XXX
  Wf_.IsDestructor_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsStatic(bool Val)
{
  assert(Wf_.isMember());
  Wf_.IsStatic_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsConst(bool Val)
{
  assert(Wf_.isMember());
  Wf_.IsConst_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsNoexcept(bool Val)
{
  Wf_.IsNoexcept_ = Val;
  return *this;
}

} // namespace cppbind
