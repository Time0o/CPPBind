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

#include "CreateWrapper.hpp"

using namespace clang::ast_matchers;

namespace cppbind
{

CreateWrapperConsumer::CreateWrapperConsumer(
  std::shared_ptr<Wrapper> Wrapper,
  std::shared_ptr<IdentifierIndex> II)
: Wr_(Wrapper),
  II_(II)
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
  for (auto const &MatcherRule : OPT(std::vector<std::string>, "wrap")) {
    auto const &[MatcherID, MatcherSource] = string::splitFirst(MatcherRule, ":");

    if (MatcherID == "const") {
      addWrapperHandler<clang::EnumDecl>(
        "enumConst",
        llvm::formatv("enumDecl("
                        "allOf("
                          "hasParent(namespaceDecl()),"
                          "{0}"
                        ")"
                      ")", MatcherSource),
        &CreateWrapperConsumer::handleEnumConst);

      addWrapperHandler<clang::VarDecl>(
        "VarConst",
        llvm::formatv("varDecl("
                        "allOf("
                          "hasStaticStorageDuration(),"
                          "hasType(isConstQualified()),"
                          "hasParent(namespaceDecl()),"
                          "{0}"
                        ")"
                      ")", MatcherSource),
        &CreateWrapperConsumer::handleVarConst);

    } else if (MatcherID == "function") {
      addWrapperHandler<clang::FunctionDecl>(
        "function",
        llvm::formatv(
          "functionDecl("
            "allOf("
              "unless(cxxMethodDecl()),"
              "anyOf(hasParent(namespaceDecl()),"
                    "allOf(isTemplateInstantiation(),"
                          "hasParent(functionTemplateDecl(hasParent(namespaceDecl()))))),"
              "{0}"
            ")"
          ")", MatcherSource),
        &CreateWrapperConsumer::handleFunction);

    } else if (MatcherID == "record") {
      addWrapperHandler<clang::CXXRecordDecl>(
        "record",
        llvm::formatv(
          "cxxRecordDecl("
            "allOf("
              "anyOf(isClass(), isStruct()),"
              "anyOf(hasParent(namespaceDecl()),"
                    "allOf(isTemplateInstantiation(),"
                          "hasParent(classTemplateDecl(hasParent(namespaceDecl()))))),"
               "{0}"
            ")"
          ")", MatcherSource),
        &CreateWrapperConsumer::handleRecord);

    } else {
      throw exception("invalid matcher: '{0}'", MatcherID);
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
    Wr_->addWrapperConstant(II_, ConstantDecl);
}

void
CreateWrapperConsumer::handleVarConst(clang::VarDecl const *Decl)
{ Wr_->addWrapperConstant(II_, Decl); }

void
CreateWrapperConsumer::handleFunction(clang::FunctionDecl const *Decl)
{ Wr_->addWrapperFunction(II_, Decl); }

void
CreateWrapperConsumer::handleRecord(clang::CXXRecordDecl const *Decl)
{ Wr_->addWrapperRecord(II_, Decl); }

} // namespace cppbind
