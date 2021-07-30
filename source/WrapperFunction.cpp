#include <algorithm>
#include <cassert>
#include <cctype>
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

#include "CompilerState.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "Print.hpp"
#include "String.hpp"
#include "TemplateArgument.hpp"
#include "WrapperFunction.hpp"
#include "WrapperObject.hpp"
#include "WrapperRecord.hpp"

namespace cppbind
{

WrapperParameter::WrapperParameter(Identifier const &Name,
                                   WrapperType const &Type,
                                   clang::Expr const *DefaultArgument)
: Name_(Name),
  Type_(Type),
  DefaultArgument_(determineDefaultArgument(DefaultArgument))
{}

WrapperParameter::WrapperParameter(clang::ParmVarDecl const *Decl)
: WrapperObject<clang::ParmVarDecl>(Decl),
  Name_(Decl),
  Type_(Decl->getType()),
  DefaultArgument_(determineDefaultArgument(Decl->getUninstantiatedDefaultArg()))
{}

std::optional<std::string>
WrapperParameter::determineDefaultArgument(clang::Expr const *DefaultExpr)
{
  if (!DefaultExpr)
    return std::nullopt;

  assert(!DefaultExpr->isInstantiationDependent()); // XXX

  return print::stmt(DefaultExpr);
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
  TemplateArgumentList_(determineTemplateArgumentList(Decl)),
  OverloadedOperator_(determineOverloadedOperator(Decl))
{}

WrapperFunction::WrapperFunction(clang::CXXMethodDecl const *Decl)
: WrapperObject<clang::FunctionDecl>(Decl),
  Origin_(Decl->getParent()->getTypeForDecl()),
  Name_(determineName(Decl)),
  ReturnType_(determineReturnType(Decl)),
  Parameters_(determineParameters(Decl)),
  IsDefinition_(Decl->isThisDeclarationADefinition()),
  IsMember_(true),
  IsConstructor_(llvm::isa<clang::CXXConstructorDecl>(Decl)),
  IsCopyConstructor_(IsConstructor_ && llvm::dyn_cast<clang::CXXConstructorDecl>(Decl)->isCopyConstructor()),
  IsCopyAssignmentOperator_(Decl->isCopyAssignmentOperator()),
  IsMoveConstructor_(IsConstructor_ && llvm::dyn_cast<clang::CXXConstructorDecl>(Decl)->isMoveConstructor()),
  IsMoveAssignmentOperator_(Decl->isMoveAssignmentOperator()),
  IsDestructor_(llvm::isa<clang::CXXDestructorDecl>(Decl)),
  IsStatic_(Decl->isStatic()),
  IsConst_(Decl->isConst()),
  IsConstexpr_(Decl->isConstexpr()),
  IsNoexcept_(determineIfNoexcept(Decl)),
  IsVirtual_(Decl->isVirtual()),
  TemplateArgumentList_(determineTemplateArgumentList(Decl)),
  OverloadedOperator_(determineOverloadedOperator(Decl))
{}

bool
WrapperFunction::operator==(WrapperFunction const &Other) const
{
  if (Name_ != Other.Name_)
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
WrapperFunction::addOverload(std::shared_ptr<IdentifierIndex> II)
{
  auto NameTemplated(getFormat(true, false, true));

  if (!II->hasOverload(NameTemplated))
    return;

  Overload_ = II->popOverload(NameTemplated);
}

Identifier
WrapperFunction::getName() const
{ return getFormat(); }

Identifier
WrapperFunction::getFormat(bool WithTemplatePostfix,
                           bool WithOverloadPostfix,
                           bool WithoutOperatorName) const
{
  Identifier Name(Name_);

  if (WithoutOperatorName && isOverloadedOperator()) {
    auto NameStr(Name.str());

    auto OOName(OverloadedOperator_->Name);

    auto OOSpelling(OverloadedOperator_->Spelling);
    if (OverloadedOperator_->IsCast)
      OOSpelling = "operator " + OOSpelling;
    else
      OOSpelling = "operator" + OOSpelling;

    string::replace(NameStr, OOSpelling, OOName);

    Name = Identifier(NameStr);
  }

  if (WithTemplatePostfix) {
    if (isMember())
        Name = Name.unqualified().qualified(getParent()->getFormat(true));

    if (isTemplateInstantiation())
        Name = Identifier(Name.str() + TemplateArgumentList_->str(true));
  }

  if (WithOverloadPostfix && isOverloaded()) {
    auto Postfix = OPT("wrap-func-overload-postfix");

    string::replaceAll(Postfix, "%o", std::to_string(*Overload_));

    Name = Identifier(Name.str() + Postfix);
  }

  return Name;
}

WrapperType
WrapperFunction::getReturnType() const
{ return ReturnType_; }

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

bool
WrapperFunction::isNoexcept() const
{ return OPT(bool, "wrap-noexcept") || isDestructor() || IsNoexcept_; }

bool
WrapperFunction::isOverriding() const
{
  if (!Parent_)
    return false;

  return Origin_ != Parent_->getType();
}

bool
WrapperFunction::isOverloadedOperator(char const *Which,
                                      int numParameters) const
{
  if (!OverloadedOperator_)
    return false;

  int numParametersActual = Parameters_.size();
  if (numParametersActual > 0 && Parameters_[0].isSelf())
    --numParametersActual;

  if (OverloadedOperator_->IsPostfix)
    ++numParametersActual;

  return (!Which || OverloadedOperator_->Spelling == Which) &&
         (numParameters == -1 || numParameters == numParametersActual);
}

bool
WrapperFunction::isCustomCast(char const *Which) const
{
  if (!OverloadedOperator_ || !OverloadedOperator_->IsCast)
    return false;

  return !Which || OverloadedOperator_->Spelling.substr(1) == Which;
}

Identifier
WrapperFunction::determineName(clang::FunctionDecl const *Decl)
{
  if (llvm::isa<clang::CXXMethodDecl>(Decl)) {
    if (llvm::isa<clang::CXXConstructorDecl>(Decl)) {
      auto const *ConstructorDecl = llvm::dyn_cast<clang::CXXConstructorDecl>(Decl);

      if (ConstructorDecl->isCopyConstructor())
        return Identifier(Identifier::COPY);
      else if (ConstructorDecl->isMoveConstructor())
        return Identifier(Identifier::MOVE);
      else
        return Identifier(Identifier::NEW);
    } else if (llvm::isa<clang::CXXDestructorDecl>(Decl)) {
      return Identifier(Identifier::DELETE);
    }
  }

  return Identifier(Decl);
}

WrapperType
WrapperFunction::determineReturnType(clang::FunctionDecl const *Decl)
{ return WrapperType(Decl->getReturnType()); }

static void
postfixParameterName(std::string &ParamName, unsigned p)
{
  auto Postfix(OPT("wrap-func-numbered-param-postfix"));

  string::replaceAll(Postfix, "%p", std::to_string(p));

  ParamName += Postfix;
}

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

  for (unsigned i = 0u; i < Params.size(); ++i) {
    ParamList.emplace_back(Identifier(ParamNames[i]),
                           WrapperType(Params[i]->getType()),
                           Params[i]->getUninstantiatedDefaultArg());
  }

  return ParamList;
}

bool
WrapperFunction::determineIfNoexcept(clang::FunctionDecl const *Decl)
{
  auto EST = Decl->getExceptionSpecType();

  return EST == clang::EST_BasicNoexcept || EST == clang::EST_NoexceptTrue;
}

std::optional<WrapperFunction::OverloadedOperator>
WrapperFunction::determineOverloadedOperator(clang::FunctionDecl const *Decl)
{
  using namespace clang;

  std::unique_ptr<OverloadedOperator> OO;

  if (!Decl->isOverloadedOperator()) {
    if (llvm::isa<clang::CXXConversionDecl>(Decl)) {
      auto ConversionDecl = llvm::dyn_cast<clang::CXXConversionDecl>(Decl);

      auto TypePostfix(print::qualType(ConversionDecl->getConversionType()));
      string::replaceAll(TypePostfix, " ", "_");

      auto Name("cast_" + TypePostfix);
      auto Spelling(Decl->getNameAsString().substr(sizeof("operator")));

      OO = std::make_unique<OverloadedOperator>(Name, Spelling);
      OO->IsCast = true;

      return *OO;
    }

    return std::nullopt;
  }

  bool OOUnary = Parameters_.empty();

  switch (Decl->getOverloadedOperator()) {
#define OVERLOADED_OPERATOR(Name,Spelling,Token,Unary,Binary,MemberOnly) \
  case OO_##Name: \
    OO = std::make_unique<OverloadedOperator>(#Name, Spelling); \
    break;
#include "clang/Basic/OperatorKinds.def"
  default:
    llvm_unreachable("invalid overloaded operator kind");
    break;
  }

  if (IsCopyAssignmentOperator_) {
    OO->Name = "copy_assign";
  } else if (IsMoveAssignmentOperator_) {
    OO->Name = "move_assign";
  } else {
    if (OO->Name == "Star") {
      OO->Name = OOUnary ? "deref" : "mult";
    } else if (OO->Name == "PlusPlus") {
      if (OOUnary) {
        OO->Name = "inc_pre";
        OO->IsPrefix = true;
      } else {
        OO->Name = "inc_post";
        OO->IsPostfix = true;
        Parameters_.pop_back();
      }
    } else if (OO->Name == "MinusMinus") {
      if (OOUnary) {
        OO->Name = "dec_pre";
        OO->IsPrefix = true;
      } else {
        OO->Name = "dec_post";
        OO->IsPostfix = true;
        Parameters_.pop_back();
      }
    } else {
      static std::unordered_map<std::string, std::string> OONames {
        {"Amp",                 "bw_and"},
        {"AmpAmp",              "and"},
        {"AmpEqual",            "bw_and_assign"},
        {"Arrow",               "access"},
        {"Call",                "call"},
        {"Caret",               "xor"},
        {"CaretEqual",          "xor_assign"},
        {"EqualEqual",          "eq"},
        {"Exclaim",             "not"},
        {"ExclaimEqual",        "ne"},
        {"Greater",             "gt"},
        {"GreaterEqual",        "ge"},
        {"GreaterGreater",      "sr"},
        {"GreaterGreaterEqual", "sr_assign"},
        {"Less",                "lt"},
        {"LessEqual",           "le"},
        {"LessLess",            "sl"},
        {"LessLessEqual",       "sl_assign"},
        {"Minus",               "minus"},
        {"MinusEqual",          "minus_assign"},
        {"Percent",             "mod"},
        {"PercentEqual",        "mod_assign"},
        {"Pipe",                "bw_or"},
        {"PipeEqual",           "bw_or_assign"},
        {"PipePipe",            "or"},
        {"Plus",                "plus"},
        {"PlusEqual",           "plus_assign"},
        {"Slash",               "div"},
        {"SlashEqual",          "div_assign"},
        {"StarEqual",           "mult_assign"},
        {"Subscript",           "idx"},
        {"Tilde",               "bw_not"}
      };

      auto It(OONames.find(OO->Name));
      if (It == OONames.end())
        throw log::exception("failed to resolve operator name '{0}'", OO->Name);

      OO->Name = It->second;
    }
  }

  return *OO;
}

std::optional<TemplateArgumentList>
WrapperFunction::determineTemplateArgumentList(clang::FunctionDecl const *Decl)
{
  try {
    return TemplateArgumentList(Decl);
  } catch (NotATemplateSpecialization const &) {
    return std::nullopt;
  }
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setOrigin(WrapperType const &Origin)
{
  Wf_.Origin_ = Origin;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setParent(WrapperRecord const *Parent)
{
  auto ParentType(Parent->getType());

  Wf_.Parent_ = Parent;
  Wf_.IsMember_ = true;

  Wf_.Name_ = Wf_.Name_.unqualified().qualified(ParentType.getName());

  if (Wf_.isInstance()) {
    WrapperType SelfType;

    if (Wf_.isDestructor() || Wf_.isConst())
      SelfType = ParentType.withConst().pointerTo();
    else
      SelfType = ParentType.pointerTo();

    WrapperParameter Self(WrapperParameter::self(SelfType));

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
WrapperFunctionBuilder::setNamespace(std::optional<Identifier> const &Namespace)
{
  Wf_.CachedNamespace_ = Namespace;
  Wf_.CachedNamespaceValid_ = true;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setPropertyFor(WrapperVariable const *PropertyFor)
{
  Wf_.PropertyFor_ = PropertyFor;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setReturnType(WrapperType const &ReturnType)
{
  Wf_.ReturnType_ = ReturnType;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setCustomAction(std::string const &CustomAction)
{
  Wf_.CustomAction_ = CustomAction;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsConstructor(bool Val)
{
  Wf_.IsConstructor_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsDestructor(bool Val)
{
  Wf_.IsDestructor_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsGetter(bool Val)
{
  Wf_.IsGetter_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsSetter(bool Val)
{
  Wf_.IsSetter_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsBaseCast(bool Val)
{
  Wf_.IsBaseCast_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsStatic(bool Val)
{
  Wf_.IsStatic_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsConst(bool Val)
{
  Wf_.IsConst_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsNoexcept(bool Val)
{
  Wf_.IsNoexcept_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsVirtual(bool Val)
{
  Wf_.IsVirtual_ = Val;
  return *this;
}

} // namespace cppbind
