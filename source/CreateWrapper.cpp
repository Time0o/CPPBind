#include <memory>
#include <string>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Sema/Sema.h"

#include "ClangUtil.hpp"
#include "CompilerState.hpp"
#include "CreateWrapper.hpp"
#include "FundamentalTypes.hpp"
#include "Identifier.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "String.hpp"
#include "Wrapper.hpp"
#include "WrapperType.hpp"

using namespace clang::ast_matchers;

namespace cppbind
{

bool
CreateWrapperVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl *Decl)
{
  // Guarantee expansion of implicit default con-/destructors so that they can
  // be matched later on. These might otherwise be omitted by Clang if they are
  // never used.

  if (CompilerState().inCurrentFile(COMPLETE_INPUT_FILE, Decl->getLocation())) {
    auto &Sema(CompilerState()->getSema());

    if (Decl->isThisDeclarationADefinition()) {
      if (Decl->needsImplicitDefaultConstructor())
        Sema.DeclareImplicitDefaultConstructor(Decl);
      if (Decl->needsImplicitDestructor())
        Sema.DeclareImplicitDestructor(Decl);
    }
  }

  return true;
}

CreateWrapperConsumer::CreateWrapperConsumer(std::shared_ptr<Wrapper> Wrapper)
: Wrapper_(Wrapper)
{
  addFundamentalTypesHandler();
  addWrapperHandlers();
}

void
CreateWrapperConsumer::addFundamentalTypesHandler()
{
  auto FundamentalTypeMatcher(
    valueDecl(hasParent(namespaceDecl(hasName("cppbind::fundamental_types")))));

  addHandler<clang::ValueDecl>(
    "fundamentalType",
    FundamentalTypeMatcher.bind("fundamentalType"),
    &CreateWrapperConsumer::handleFundamentalType);
}

void
CreateWrapperConsumer::addWrapperHandlers()
{
  auto OrigInputFile(CompilerState().currentFile(ORIG_INPUT_FILE, false));
  auto TmpInputFile(CompilerState().currentFile(TMP_INPUT_FILE, false));

  enum MatchLocation
  {
    ANYWHERE,
    INSIDE_SOURCE,
    OUTSIDE_SOURCE
  };

  // Match declaration in the input source file.
  std::string InsideSource(
    llvm::formatv("anyOf(isExpansionInFileMatching(\"{0}\"),"
                        "isExpansionInFileMatching(\"{1}\"))",
                  OrigInputFile,
                  TmpInputFile));

  // Match declaration anywhere else in the translation unit.
  std::string OutsideSource(
    llvm::formatv("unless({0})",
                  InsideSource));

  // Process wrap rules passed as command line options via --wrap-rule.
  for (auto const &MatcherRule : OPT(std::vector<std::string>, "wrap-rule")) {
    auto Tmp(string::splitFirst(MatcherRule, ":"));

    auto MatcherID(Tmp.first);
    auto MatcherSource(Tmp.second);

    auto match = [&](MatchLocation Where,
                     std::string const &DeclType,
                     std::string const &RestrictionsFmt,
                     auto ...RestrictionsFmtArgs)
    {
      auto MatcherRestrictions(llvm::formatv(RestrictionsFmt.c_str(),
                                             RestrictionsFmtArgs...));

      switch (Where) {
      case ANYWHERE:
        return static_cast<std::string>(
          llvm::formatv("{0}(allOf({1}, {2}))",
                        DeclType,
                        MatcherRestrictions,
                        MatcherSource));
      case INSIDE_SOURCE:
        return static_cast<std::string>(
          llvm::formatv("{0}(allOf({1}, {2}, {3}))",
                        DeclType,
                        InsideSource,
                        MatcherRestrictions,
                        MatcherSource));
      case OUTSIDE_SOURCE:
        return static_cast<std::string>(
          llvm::formatv("{0}(allOf({1}, {2}, {3}))",
                        DeclType,
                        OutsideSource,
                        MatcherRestrictions,
                        MatcherSource));
      }
    };

    if (MatcherID == "enum") {
      addWrapperHandler<clang::EnumDecl>(
        "enum",
        match(INSIDE_SOURCE, "enumDecl", matchEnum()),
        declHandler<&CreateWrapperConsumer::handleEnum>());

    } else if (MatcherID == "variable") {
      addWrapperHandler<clang::VarDecl>(
        "variable",
        match(INSIDE_SOURCE, "varDecl", matchVariable()),
        declHandler<&CreateWrapperConsumer::handleVariable>());

    } else if (MatcherID == "function") {
      addWrapperHandler<clang::FunctionDecl>(
        "function",
        match(INSIDE_SOURCE, "functionDecl", matchFunction()),
        declHandler<&CreateWrapperConsumer::handleFunction>());

    } else if (MatcherID == "record") {
      addWrapperHandler<clang::CXXRecordDecl>(
        "recordDeclaration",
        match(OUTSIDE_SOURCE, "cxxRecordDecl", matchRecordDeclaration()),
        declHandler<&CreateWrapperConsumer::handleRecordDeclaration>());

      addWrapperHandler<clang::CXXRecordDecl>(
        "recordDefinition",
        match(INSIDE_SOURCE, "cxxRecordDecl", matchRecordDefinition()),
        declHandler<&CreateWrapperConsumer::handleRecordDefinition>());

    } else {
      throw log::exception("invalid matcher: '{0}'", MatcherID);
    }
  }
}

std::string
CreateWrapperConsumer::matchToplevel()
{ return "hasParent(anyOf(namespaceDecl(), translationUnitDecl()))"; }

std::string
CreateWrapperConsumer::matchNested()
{ return "allOf(hasParent(cxxRecordDecl()), isPublic(), unless(isImplicit()))"; }

std::string
CreateWrapperConsumer::matchToplevelOrNested()
{ return llvm::formatv("anyOf({0}, {1})", matchToplevel(), matchNested()); }

std::string
CreateWrapperConsumer::matchEnum()
{ return llvm::formatv("{0}", matchToplevelOrNested()); }

std::string
CreateWrapperConsumer::matchVariable()
{ return matchToplevel(); }

std::string
CreateWrapperConsumer::matchFunction()
{
  return llvm::formatv(
           "allOf(unless(cxxMethodDecl()),"
                 "anyOf({0},"
                       "allOf(isTemplateInstantiation(),"
                             "hasParent(functionTemplateDecl({0})))))",
           matchToplevel());
}

std::string
CreateWrapperConsumer::matchRecordDeclaration()
{
  return llvm::formatv(
           "anyOf({0},"
                 "allOf(isTemplateInstantiation(),"
                       "hasParent(classTemplateDecl({0}))))",
           matchToplevelOrNested());
}

std::string
CreateWrapperConsumer::matchRecordDefinition()
{
  return llvm::formatv(
           "anyOf({0},"
                 "allOf(isTemplateInstantiation(),"
                       "hasParent(classTemplateDecl({0}))))",
           matchToplevelOrNested());
}

void
CreateWrapperConsumer::handleFundamentalType(clang::ValueDecl const *Decl)
{
  auto const *Type = Decl->getType().getTypePtr();

  if (Type->isPointerType())
    Type = Type->getPointeeType().getTypePtr();

  FundamentalTypes().add(Type);
}

void
CreateWrapperConsumer::handleEnum(clang::EnumDecl const *Decl)
{ Wrapper_->addWrapperEnum(Decl); }

void
CreateWrapperConsumer::handleVariable(clang::VarDecl const *Decl)
{ Wrapper_->addWrapperVariable(Decl); }

void
CreateWrapperConsumer::handleFunction(clang::FunctionDecl const *Decl)
{ Wrapper_->addWrapperFunction(Decl); }

void
CreateWrapperConsumer::handleRecordDeclaration(clang::CXXRecordDecl const *Decl)
{ Wrapper_->addWrapperRecord(Decl, false); }

void
CreateWrapperConsumer::handleRecordDefinition(clang::CXXRecordDecl const *Decl)
{
  if (Decl->isThisDeclarationADefinition()) {
    // Match member declarations before the record definition itself.
    auto DeclContext = static_cast<clang::DeclContext const *>(Decl);
    for (auto const *InnerDecl : DeclContext->decls())
      matchDecl(InnerDecl);
  }

  Wrapper_->addWrapperRecord(Decl);
}

void
CreateWrapperFrontendAction::beforeProcessing()
{
  InputFile_ = CompilerState().currentFile(ORIG_INPUT_FILE);

  Wrapper_ = std::make_shared<Wrapper>();
}

void
CreateWrapperFrontendAction::afterProcessing()
{
  for (auto const &Include : includes())
    Wrapper_->addInclude(Include.first, Include.second);

  if (OPT(bool, "wrap-macro-constants")) {
    for (auto const &Definition : definitions())
      Wrapper_->addMacro(Identifier(Definition.first), Definition.second);
  }

  Wrapper_->addOverloads();

  backend::run(InputFile_, Wrapper_);
}

void
CreateWrapperToolRunner::adjustArguments(
  std::vector<clang::tooling::ArgumentsAdjuster> &ArgumentsAdjusters) const
{
  // Always include 'generate/cppbind/fundamental_types.h' into the translation
  // unit to guarantee that all fundamental types are available even if not
  // present in the latter.
  insertArguments({"-include", FUNDAMENTAL_TYPES_HEADER}, ArgumentsAdjusters);
}

std::unique_ptr<clang::tooling::FrontendActionFactory>
CreateWrapperToolRunner::makeFactory() const
{ return makeSimpleFactory<CreateWrapperFrontendAction>(); }

} // namespace cppbind
