#include <memory>
#include <string>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchers.h"

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
    "fundamentalTypeDecl",
    FundamentalTypeMatcher.bind("fundamentalTypeDecl"),
    &CreateWrapperConsumer::handleFundamentalTypeDecl);
}

void
CreateWrapperConsumer::addWrapperHandlers()
{
  auto InputFile(CompilerState().currentFile());

  for (auto const &MatcherRule : OPT(std::vector<std::string>, "wrap-rule")) {
    auto Tmp(string::splitFirst(MatcherRule, ":"));

    auto MatcherID(Tmp.first);
    auto MatcherSource(Tmp.second);

    auto match = [&](std::string const &DeclType,
                     std::string const &RestrictionsFmt,
                     auto ...RestrictionsFmtArgs)
    {
      auto MatcherRestrictions(
        llvm::formatv(RestrictionsFmt.c_str(), RestrictionsFmtArgs...));

      return static_cast<std::string>(
        llvm::formatv(
          "{0}(allOf(isExpansionInFileMatching(\"{1}\"), {2}, {3}))",
          DeclType, InputFile, MatcherRestrictions, MatcherSource));
    };

    char const *MatchToplevel =
      "hasParent(anyOf(namespaceDecl(), translationUnitDecl()))";

    char const *MatchNested =
      "allOf(isPublic(), unless(isImplicit()))";

    std::string MatchToplevelOrNested(
      llvm::formatv("anyOf({0}, {1})", MatchToplevel, MatchNested));

    if (MatcherID == "const") {
      addWrapperHandler<clang::EnumDecl>(
        "enumConst",
        match("enumDecl", MatchToplevel),
        &CreateWrapperConsumer::handleEnumConst);

      addWrapperHandler<clang::VarDecl>(
        "VarConst",
        match("varDecl", "allOf(isConstexpr(), {0})", MatchToplevel),
        &CreateWrapperConsumer::handleVarConst);

    } else if (MatcherID == "function") {
      addWrapperHandler<clang::FunctionDecl>(
        "function",
        match("functionDecl",
              "allOf(unless(cxxMethodDecl()),"
                    "anyOf({0},"
                          "allOf(isTemplateInstantiation(),"
                                "hasParent(functionTemplateDecl({0})))))",
              MatchToplevel),
        &CreateWrapperConsumer::handleFunction);

    } else if (MatcherID == "record") {
      addWrapperHandler<clang::CXXRecordDecl>(
        "record",
        match("cxxRecordDecl",
              "anyOf({0},"
                    "allOf(isTemplateInstantiation(),"
                          "hasParent(classTemplateDecl({0}))))",
              MatchToplevelOrNested),
        &CreateWrapperConsumer::handleRecord);

    } else {
      throw log::exception("invalid matcher: '{0}'", MatcherID);
    }
  }
}

void
CreateWrapperConsumer::handleFundamentalTypeDecl(clang::ValueDecl const *Decl)
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
CreateWrapperConsumer::handleRecord(clang::CXXRecordDecl const *Decl)
{ Wrapper_->addWrapperRecord(Decl); }

void
CreateWrapperFrontendAction::beforeProcessing()
{
  InputFile_ = CompilerState().currentFile();

  Wrapper_ = std::make_shared<Wrapper>(II_, TI_);
}

void
CreateWrapperFrontendAction::afterProcessing()
{
  Wrapper_->finalize();

  Backend::run(InputFile_, Wrapper_);
}

std::unique_ptr<clang::tooling::FrontendActionFactory>
CreateWrapperToolRunner::makeFactory() const
{ return makeFactoryWithArgs<CreateWrapperFrontendAction>(II_, TI_); }

} // namespace cppbind
