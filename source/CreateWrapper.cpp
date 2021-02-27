#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchers.h"

#include "llvm/Support/FormatVariadic.h"

#include "ClangUtil.hpp"
#include "FundamentalTypes.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Options.hpp"
#include "String.hpp"
#include "Wrapper.hpp"
#include "WrapperType.hpp"

#include "CreateWrapper.hpp"

using namespace clang::ast_matchers;

namespace cppbind
{

CreateWrapperConsumer::CreateWrapperConsumer(
  std::shared_ptr<IdentifierIndex> II,
  std::shared_ptr<Wrapper> Wrapper)
: II_(II),
  Wr_(Wrapper)
{
  addFundamentalTypesHandler();
  addWrapperHandlers();
}

void
CreateWrapperConsumer::addFundamentalTypesHandler()
{
  auto FundamentalTypeMatcher(
    valueDecl(hasParent(namespaceDecl(hasName("__fundamental_types")))));

  addHandler<clang::ValueDecl>(
    "fundamentalTypeDecl",
    FundamentalTypeMatcher.bind("fundamentalTypeDecl"),
    &CreateWrapperConsumer::handleFundamentalTypeDecl);
}

void
CreateWrapperConsumer::addWrapperHandlers()
{
  for (auto const &MatcherRule : OPT(std::vector<std::string>, "match")) {
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
        llvm::formatv("cxxRecordDecl("
                        "allOf("
                          "anyOf(isClass(), isStruct()),"
                          "hasParent(namespaceDecl()),"
                          "{0}"
                        ")"
                      ")", MatcherSource),
        &CreateWrapperConsumer::handleRecord);

    } else {
      throw std::invalid_argument(llvm::formatv("invalid matcher: '{0}'", MatcherID));
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
    Wr_->addWrapperVariable(II_, Identifier(ConstantDecl), WrapperType(Decl));
}

void
CreateWrapperConsumer::handleVarConst(clang::VarDecl const *Decl)
{ Wr_->addWrapperVariable(II_, Decl); }

void
CreateWrapperConsumer::handleFunction(clang::FunctionDecl const *Decl)
{
  if (!clang_util::isMethod(Decl)) // XXX exclude this case in matcher expression
    Wr_->addWrapperFunction(II_, Decl);
}

void
CreateWrapperConsumer::handleRecord(clang::CXXRecordDecl const *Decl)
{ Wr_->addWrapperRecord(II_, Decl); }

} // namespace cppbind
