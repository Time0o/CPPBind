#include "WrapperFunction.hpp"

#include <cassert>
#include <deque>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
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

#include "ClangUtil.hpp"
#include "CompilerState.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "String.hpp"
#include "Util.hpp"
#include "WrapperObject.hpp"
#include "WrapperRecord.hpp"

namespace cppbind
{

WrapperParameter::WrapperParameter(Identifier const &Name,
                                   WrapperType const &Type)
: Name_(Name),
  Type_(Type)
{}

WrapperParameter::WrapperParameter(Identifier const &Name,
                                   clang::ParmVarDecl const *Decl)
: WrapperObject<clang::ParmVarDecl>(Decl),
  Name_(Name),
  Type_(Decl->getType())
{
  auto const *DefaultExpr = Decl->getDefaultArg();

  if (DefaultExpr)
    DefaultArgument_ = DefaultArgument(DefaultExpr);
}

WrapperParameter::DefaultArgument::DefaultArgument(clang::Expr const *Expr)
{
  auto &Ctx(ASTContext());

  if (Expr->HasSideEffects(Ctx))
    exception("default value must not have side effects");

  if (Expr->isValueDependent() || Expr->isTypeDependent())
    exception("default value must not be value/type dependent");

  if (Expr->isNullPointerConstant(Ctx, clang::Expr::NPC_NeverValueDependent)) {
    Value_ = nullptr;
    return;
  }

  clang::Expr::EvalResult Result;
  if (!Expr->EvaluateAsRValue(Result, Ctx, true))
    exception("default value must be constant foldable to rvalue");

  switch(Result.Val.getKind()) {
  case clang::APValue::Int:
    Value_ = Result.Val.getInt();
    break;
  case clang::APValue::Float:
    Value_ = Result.Val.getFloat();
    break;
  default:
    exception("default value must have pointer, integer or floating point type"); // XXX
  }
}

std::string
WrapperParameter::DefaultArgument::str() const
{
  if (isNullptrT())
    return "nullptr";
  else if (isInt())
    return APToString(as<llvm::APSInt>());
  else if (isFloat())
    return APToString(as<llvm::APFloat>());

  llvm_unreachable("invalid default argument type");
}

WrapperFunction::WrapperFunction(Identifier const &Name)
: Name_(Name),
  ReturnType_("void")
{}

WrapperFunction::WrapperFunction(clang::FunctionDecl const *Decl)
: WrapperObject<clang::FunctionDecl>(Decl),
  Name_(determineName(Decl)),
  ReturnType_(determineReturnType(Decl)),
  Parameters_(determineParameters(Decl)),
  IsDefinition_(Decl->isThisDeclarationADefinition()),
  IsNoexcept_(determineIfNoexcept(Decl)),
  OverloadedOperator_(determineOverloadedOperator(Decl)),
  TemplateArgumentList_(determineTemplateArgumentList(Decl))
{}

WrapperFunction::WrapperFunction(clang::CXXMethodDecl const *Decl)
: WrapperObject<clang::FunctionDecl>(Decl),
  Name_(determineName(Decl)),
  ReturnType_(determineReturnType(Decl)),
  Parameters_(determineParameters(Decl)),
  IsDefinition_(Decl->isThisDeclarationADefinition()),
  IsConstructor_(clang_util::isConstructor(Decl)),
  IsDestructor_(clang_util::isDestructor(Decl)),
  IsStatic_(Decl->isStatic()),
  IsConst_(Decl->isConst()),
  IsNoexcept_(determineIfNoexcept(Decl)),
  OverloadedOperator_(determineOverloadedOperator(Decl)),
  TemplateArgumentList_(determineTemplateArgumentList(Decl))
{}

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

  if (TemplateArgumentList_ != Other.TemplateArgumentList_)
    return false;

  return true;
}

void
WrapperFunction::overload(std::shared_ptr<IdentifierIndex> II)
{
  auto NameTemplated(getName(false, false, true));

  if (!II->hasOverload(NameTemplated))
    return;

  Overload_ = II->popOverload(NameTemplated);
}

Identifier
WrapperFunction::getName(bool Overloaded,
                         bool WithoutOperatorName,
                         bool WithTemplatePostfix) const
{
  Identifier Name(Name_);

  if (WithoutOperatorName && isOverloadedOperator()) {
    auto NameStr(Name.str());

    string::replace(NameStr,
                    OverloadedOperator_->Spelling,
                    OverloadedOperator_->Name);

    Name = Identifier(NameStr);
  }

  if (WithTemplatePostfix) {
    if (isMember() && Parent_->isTemplateInstantiation())
      Name = Name.unqualified().qualified(Parent_->getName(true));

    if (isTemplateInstantiation())
      Name = Identifier(Name.str() + TemplateArgumentList_->str(true));
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
  if (!isOverloadedOperator())
    return std::nullopt;

  return OverloadedOperator_->Name;
}

std::optional<std::string>
WrapperFunction::getTemplateArgumentList() const
{
  if (!isTemplateInstantiation())
    return std::nullopt;

  return TemplateArgumentList_->str();
}

Identifier
WrapperFunction::determineName(clang::FunctionDecl const *Decl)
{
  if (clang_util::isConstructor(Decl))
    return Identifier(Identifier::NEW);

  if (clang_util::isDestructor(Decl))
    return Identifier(Identifier::DELETE);

  return Identifier(Decl);
}

WrapperType
WrapperFunction::determineReturnType(clang::FunctionDecl const *Decl)
{ return WrapperType(Decl->getReturnType()); }

std::deque<WrapperParameter>
WrapperFunction::determineParameters(clang::FunctionDecl const *Decl)
{
  auto Params(Decl->parameters());

  // determine parameter names
  std::deque<std::string> ParamNames;

  std::unordered_map<std::string, std::pair<unsigned, unsigned>> ParamNameCounts;

  for (unsigned i = 0u; i < Params.size(); ++i) {
    auto ParamName(Params[i]->getNameAsString());

    if (ParamName.empty()) {
      postfixParameterName(ParamName, i + 1u);

    } else {
      auto It(ParamNameCounts.find(ParamName));

      if (It == ParamNameCounts.end()) {
        ParamNameCounts[ParamName] = std::make_pair(i, 1);

      } else {
        auto [FirstOccurrence, Count] = It->second;

        ParamNameCounts[ParamName] = std::make_pair(FirstOccurrence, ++Count);

        if (Count == 2)
          postfixParameterName(ParamNames[FirstOccurrence], 1);

        postfixParameterName(ParamName, Count);
      }
    }

    ParamNames.push_back(ParamName);
  }

  // construct parameter list
  std::deque<WrapperParameter> ParamList;

  for (unsigned i = 0u; i < Params.size(); ++i)
    ParamList.emplace_back(Identifier(ParamNames[i]), Params[i]);

  return ParamList;
}

void
WrapperFunction::postfixParameterName(std::string &ParamName, unsigned p)
{
  auto Postfix(OPT("wrapper-func-numbered-param-postfix"));

  string::replaceAll(Postfix, "%p", std::to_string(p));

  ParamName += Postfix;
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

std::optional<clang_util::TemplateArgumentList>
WrapperFunction::determineTemplateArgumentList(clang::FunctionDecl const *Decl)
{
  if (!Decl->isTemplateInstantiation())
    return std::nullopt;

  auto Args = Decl->getTemplateSpecializationArgs();
  if (!Args)
    return std::nullopt;

  return clang_util::TemplateArgumentList(*Args);
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
