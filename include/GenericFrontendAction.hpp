#ifndef GUARD_GENERIC_FRONTEND_ACTION_H
#define GUARD_GENERIC_FRONTEND_ACTION_H

#include <memory>

#include "clang/Frontend/FrontendAction.h"

#include "llvm/ADT/StringRef.h"

#include "CompilerState.hpp"

namespace clang
{
  class ASTConsumer;
  class CompilerInstance;
}

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

  virtual void beforeProcessing() {};
  virtual void afterProcessing() {};
};

} // namespace cppbind

#endif // GUARD_GENERIC_FRONTEND_ACTION_H
