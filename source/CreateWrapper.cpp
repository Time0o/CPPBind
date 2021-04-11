#include <memory>
#include <string>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Sema/Sema.h"

#include "ClangUtil.hpp"
#include "FundamentalTypes.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "String.hpp"
#include "Wrapper.hpp"
#include "WrapperType.hpp"

#include "CompilerState.hpp"
#include "CreateWrapper.hpp"

using namespace clang::ast_matchers;

namespace cppbind
{

bool
CreateWrapperVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl *Decl)
{
  if (inInputFile(Decl)) {
    auto &Sema(CompilerState()->getSema());

    if (Decl->isThisDeclarationADefinition()) {
      if (Decl->needsImplicitDefaultConstructor())
        Sema.DeclareImplicitDefaultConstructor(Decl);
      if (Decl->needsImplicitDestructor())
        Sema.DeclareImplicitDestructor(Decl);
    }
  } else {
    Decl->setCompleteDefinition(false);
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
  auto OrigInputFile(CompilerState().currentFile(false, true));
  auto TmpInputFile(CompilerState().currentFile(true, true));

  for (auto const &MatcherRule : OPT(std::vector<std::string>, "wrap-rule")) {
    auto Tmp(string::splitFirst(MatcherRule, ":"));

    auto MatcherID(Tmp.first);
    auto MatcherSource(Tmp.second);

    auto matchAnywhere = [&](std::string const &DeclType,
                             std::string const &RestrictionsFmt,
                             auto ...RestrictionsFmtArgs)
    {
      auto MatcherRestrictions(
        llvm::formatv(RestrictionsFmt.c_str(), RestrictionsFmtArgs...));

      return static_cast<std::string>(
        llvm::formatv("{0}(allOf({1}, {2}))",
                      DeclType,
                      MatcherRestrictions,
                      MatcherSource));
    };

    auto matchInSource = [&](std::string const &DeclType,
                             std::string const &RestrictionsFmt,
                             auto ...RestrictionsFmtArgs)
    {
      auto MatcherRestrictions(
        llvm::formatv(RestrictionsFmt.c_str(), RestrictionsFmtArgs...));

      std::string InSource(
        llvm::formatv("anyOf(isExpansionInFileMatching(\"{0}\"),"
                            "isExpansionInFileMatching(\"{1}\"))",
                      OrigInputFile,
                      TmpInputFile));

      return static_cast<std::string>(
        llvm::formatv("{0}(allOf({1}, {2}, {3}))",
                      DeclType,
                      InSource,
                      MatcherRestrictions,
                      MatcherSource));
    };

    char const *MatchToplevel =
      "hasParent(anyOf(namespaceDecl(), translationUnitDecl()))";

    char const *MatchNested =
      "allOf(hasParent(cxxRecordDecl()), isPublic(), unless(isImplicit()))";

    std::string MatchToplevelOrNested(
      llvm::formatv("anyOf({0}, {1})", MatchToplevel, MatchNested));

    if (MatcherID == "const") {
      addWrapperHandler<clang::EnumDecl>(
        "enumConst",
        matchInSource("enumDecl", MatchToplevelOrNested),
        declHandler<&CreateWrapperConsumer::handleEnumConst>());

      addWrapperHandler<clang::VarDecl>(
        "VarConst",
        matchInSource("varDecl", "allOf(isConstexpr(), {0})", MatchToplevelOrNested),
        declHandler<&CreateWrapperConsumer::handleVarConst>());

    } else if (MatcherID == "function") {
      addWrapperHandler<clang::FunctionDecl>(
        "function",
        matchInSource("functionDecl",
                      "allOf(unless(cxxMethodDecl()),"
                            "anyOf({0},"
                                  "allOf(isTemplateInstantiation(),"
                                        "hasParent(functionTemplateDecl({0})))))",
                      MatchToplevel),
        declHandler<&CreateWrapperConsumer::handleFunction>());

    } else if (MatcherID == "record") {
      addWrapperHandler<clang::CXXRecordDecl>(
        "recordDeclaration",
        matchAnywhere("cxxRecordDecl",
                      "allOf(unless(isDefinition()),"
                            "anyOf({0},"
                                  "allOf(isTemplateInstantiation(),"
                                        "hasParent(classTemplateDecl({0})))))",
                      MatchToplevelOrNested),
        declHandler<&CreateWrapperConsumer::handleRecordDeclaration>());

      addWrapperHandler<clang::CXXRecordDecl>(
        "recordDefinition",
        matchInSource("cxxRecordDecl",
                      "allOf(isDefinition(),"
                            "anyOf({0},"
                                  "allOf(isTemplateInstantiation(),"
                                        "hasParent(classTemplateDecl({0})))))",
                      MatchToplevelOrNested),
        declHandler<&CreateWrapperConsumer::handleRecordDefinition>());

    } else {
      throw log::exception("invalid matcher: '{0}'", MatcherID);
    }
  }
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
CreateWrapperConsumer::handleEnumConst(clang::EnumDecl const *Decl)
{
  for (auto const &ConstantDecl : Decl->enumerators())
    Wrapper_->addWrapperConstant(ConstantDecl);
}

void
CreateWrapperConsumer::handleVarConst(clang::VarDecl const *Decl)
{ Wrapper_->addWrapperConstant(Decl); }

void
CreateWrapperConsumer::handleFunction(clang::FunctionDecl const *Decl)
{ Wrapper_->addWrapperFunction(Decl); }

void
CreateWrapperConsumer::handleRecordDeclaration(clang::CXXRecordDecl const *Decl)
{ Wrapper_->addWrapperRecord(Decl); }

void
CreateWrapperConsumer::handleRecordDefinition(clang::CXXRecordDecl const *Decl)
{
  auto DeclContext = static_cast<clang::DeclContext const *>(Decl);
  for (auto const *InnerDecl : DeclContext->decls())
    matchDecl(InnerDecl);

  Wrapper_->addWrapperRecord(Decl);
}

void
CreateWrapperFrontendAction::beforeProcessing()
{
  InputFile_ = CompilerState().currentFile();

  Wrapper_ = std::make_shared<Wrapper>(II_, TI_);
}

void
CreateWrapperFrontendAction::afterProcessing()
{
  Wrapper_->addIncludes(includes().begin(), includes().end());

  Wrapper_->finalize();

  Backend::run(InputFile_, Wrapper_);
}

std::unique_ptr<clang::tooling::FrontendActionFactory>
CreateWrapperToolRunner::makeFactory() const
{ return makeFactoryWithArgs<CreateWrapperFrontendAction>(II_, TI_); }

} // namespace cppbind
