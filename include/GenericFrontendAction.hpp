#ifndef GUARD_GENERIC_FRONTEND_ACTION_H
#define GUARD_GENERIC_FRONTEND_ACTION_H

#include <iostream>
#include <memory>

#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"

#include "llvm/ADT/StringRef.h"

namespace cppbind
{

template<typename CONSUMER>
class GenericFrontendAction : public clang::ASTFrontendAction
{
public:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &CI,
    llvm::StringRef File) override
  {
    CompilerState().updateCompilerInstance(CI);
    CompilerState().updateFile(File.str());

    beforeProcessing();

    return makeConsumer();
  }

  void EndSourceFileAction() override
  { afterProcessing(); }

private:
  virtual std::unique_ptr<CONSUMER> makeConsumer() = 0;

  virtual void beforeProcessing() = 0;
  virtual void afterProcessing() = 0;
};

} // namespace cppbind

#endif // GUARD_GENERIC_FRONTEND_ACTION_H
