#ifndef GUARD_GENERIC_FRONTEND_ACTION_H
#define GUARD_GENERIC_FRONTEND_ACTION_H

#include <cassert>
#include <deque>
#include <memory>
#include <utility>

#include "clang/Basic/Module.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Token.h"

#include "llvm/ADT/StringRef.h"

#include "CompilerState.hpp"
#include "Include.hpp"
#include "Logging.hpp"
#include "Print.hpp"

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
  using Definition = std::pair<std::string, std::string>;

  class PPCallback : public clang::PPCallbacks
  {
  public:
    PPCallback(std::deque<Include> *Includes,
                          std::deque<Definition> *Definitions)
    : Includes_(Includes),
      Definitions_(Definitions)
    {}

    void InclusionDirective(
      clang::SourceLocation Loc,
      clang::Token const &,
      llvm::StringRef,
      bool IsAngled,
      clang::CharSourceRange,
      clang::FileEntry const *FileEntry,
      llvm::StringRef,
      llvm::StringRef,
      clang::Module const *,
      clang::SrcMgr::CharacteristicKind) override
    {
      if (!CompilerState().inCurrentFile(TMP_INPUT_FILE, Loc))
        return;

      auto IncludePath(FileEntry->getName().str());
      if (IncludePath == CompilerState().currentFile(TMP_INPUT_FILE))
        return;

      Includes_->emplace_back(IncludePath, IsAngled);
    }

    void MacroDefined(
      clang::Token const &MT,
      clang::MacroDirective const *MD) override
    {
      auto Loc = MT.getLocation();
      auto EndLoc = MT.getEndLoc();
      auto Name(print::sourceContent(Loc, EndLoc));

      if (!CompilerState().inCurrentFile(ORIG_INPUT_FILE, Loc))
        return;

      auto MI = MD->getMacroInfo();

      if (!MI->isObjectLike() || MI->tokens_empty())
        return;

      auto ArgLoc = MI->tokens().front().getLocation();
      auto ArgEndLoc = MI->tokens().back().getEndLoc();
      auto Arg(print::sourceContent(ArgLoc, ArgEndLoc));

      if (Arg.find_first_not_of("0123456789()+-*/%<>~&|^!= ") == std::string::npos)
        Definitions_->emplace_back(Name, Arg);
    }

  private:
    std::deque<Include> *Includes_;
    std::deque<Definition> *Definitions_;
  };

public:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &CI,
    llvm::StringRef File) override
  {
    updateCompilerState(CI, File);

    createIncludeMatcher(CI);

    beforeProcessing();

    return makeConsumer();
  }

  void EndSourceFileAction() override
  { afterProcessing(); }

private:
  static void updateCompilerState(clang::CompilerInstance &CI,
                                  llvm::StringRef File)
  {
    CompilerState().updateCompilerInstance(CI);
    CompilerState().updateFile(File.str());

    log::info("processing input file '{0}'",
              CompilerState().currentFile(ORIG_INPUT_FILE));
  }

  virtual std::unique_ptr<CONSUMER> makeConsumer() = 0;

  virtual void beforeProcessing() {};
  virtual void afterProcessing() {};

protected:
  std::deque<Include> const &includes() const
  { return Includes_; }

  std::deque<Definition> const &definitions() const
  { return Definitions_; }

private:
  void createIncludeMatcher(clang::CompilerInstance &CI)
  {
    Includes_.clear();

    clang::Preprocessor &PP = CI.getPreprocessor();

    PP.addPPCallbacks(std::make_unique<PPCallback>(&Includes_, &Definitions_));
  }

  std::deque<Include> Includes_;
  std::deque<Definition> Definitions_;
};

} // namespace cppbind

#endif // GUARD_GENERIC_FRONTEND_ACTION_H
