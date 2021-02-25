#include <memory>
#include <string>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchers.h"

#include "ClangUtils.hpp"
#include "Error.hpp"
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
        string::Builder() << "enumDecl("
                          <<   "allOf("
                          <<     "hasParent(namespaceDecl()),"
                          <<     MatcherSource
                          <<   ")"
                          << ")",
        &CreateWrapperConsumer::handleEnumConst);

      addWrapperHandler<clang::VarDecl>(
        "VarConst",
        string::Builder() << "varDecl("
                          <<   "allOf("
                          <<     "hasStaticStorageDuration(),"
                          <<     "hasType(isConstQualified()),"
                          <<     "hasParent(namespaceDecl()),"
                          <<     MatcherSource
                          <<   ")"
                          << ")",
        &CreateWrapperConsumer::handleVarConst);

    } else if (MatcherID == "function") {
      addWrapperHandler<clang::FunctionDecl>(
        "function",
        string::Builder() << "functionDecl("
                          <<   "allOf("
                          <<     "hasParent(namespaceDecl()),"
                          <<     MatcherSource
                          <<   ")"
                          << ")",
        &CreateWrapperConsumer::handleFunction);

    } else if (MatcherID == "record") {
      addWrapperHandler<clang::CXXRecordDecl>(
        "record",
        string::Builder() << "cxxRecordDecl("
                          <<   "allOf("
                          <<     "anyOf(isClass(), isStruct()),"
                          <<     "hasParent(namespaceDecl()),"
                          <<      MatcherSource
                          <<   ")"
                          << ")",
        &CreateWrapperConsumer::handleRecord);

    } else {
      throw CPPBindError(string::Builder() << "invalid matcher: '" << MatcherID << "'");
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
  if (!decl::isMethod(Decl))
    Wr_->addWrapperFunction(II_, Decl);
}

void
CreateWrapperConsumer::handleRecord(clang::CXXRecordDecl const *Decl)
{ Wr_->addWrapperRecord(II_, Decl); }

} // namespace cppbind
