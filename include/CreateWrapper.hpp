#ifndef GUARD_CREATE_WRAPPER_H
#define GUARD_CREATE_WRAPPER_H

#include <filesystem>
#include <memory>
#include <string>

#include "clang/ASTMatchers/ASTMatchers.h"

#include "BuiltinTypes.hpp"
#include "CompilerState.hpp"
#include "GenericASTConsumer.hpp"
#include "GenericFrontendAction.hpp"
#include "Options.hpp"
#include "WrapperHeader.hpp"

namespace cppbind
{

class CreateWrapperConsumer : public GenericASTConsumer
{
public:
  CreateWrapperConsumer(std::shared_ptr<WrapperHeader> WH)
  : _WH(WH)
  {}

private:
  void addHandlers() override
  {
    using namespace clang::ast_matchers;

    auto inNS = hasParent(namespaceDecl(hasName(NAMESPACE)));

    addHandler<clang::BuiltinType>(
      "builtinType",
      builtinType(),
      &CreateWrapperConsumer::handleBuiltinType);

    addHandler<clang::CXXRecordDecl>(
      "classDecl",
      cxxRecordDecl(allOf(inNS, anyOf(isClass(), isStruct()))),
      &CreateWrapperConsumer::handleClassDecl);

    addHandler<clang::CXXMethodDecl>(
      "publicMethodDecl",
      cxxMethodDecl(allOf(isPublic(), hasParent(cxxRecordDecl(inNS)))),
      &CreateWrapperConsumer::handlePublicMethodDecl);

    addHandler<clang::FunctionDecl>(
      "nonClassFunctionDecl",
      functionDecl(inNS),
      &CreateWrapperConsumer::handleNonClassFunctionDecl);
  }

private:
  void handleBuiltinType(clang::BuiltinType const *Type)
  { addBuiltinType(Type); }

  void handleClassDecl(clang::CXXRecordDecl const *Decl)
  { _WH->addWrapperRecord(Decl); }

  void handlePublicMethodDecl(clang::CXXMethodDecl const *Decl)
  { _WH->addWrapperFunction(Decl); }

  void handleNonClassFunctionDecl(clang::FunctionDecl const *Decl)
  { _WH->addWrapperFunction(Decl); }

  std::shared_ptr<WrapperHeader> _WH;
};

// XXX what about parallel invocations?
class CreateWrapperFrontendAction
: public GenericFrontendAction<CreateWrapperConsumer>
{
private:
  std::unique_ptr<CreateWrapperConsumer> makeConsumer() override
  {
    // XXX skip source files, filter headers?

    return std::make_unique<CreateWrapperConsumer>(_WH);
  }

  void beforeSourceFile() override
  { _WH = std::make_shared<WrapperHeader>(); }

  void afterSourceFile() override
  {
    auto WrappedFile(CompilerState().currentFile());

    // XXX write should take path
    _WH->headerFile(WrappedFile).write();
    _WH->sourceFile(WrappedFile).write();
  }

  std::shared_ptr<WrapperHeader> _WH;
};

} // namespace cppbind

#endif // GUARD_CREATE_WRAPPER_H
