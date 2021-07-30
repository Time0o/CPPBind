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
#include "Logging.hpp"
#include "Print.hpp"

namespace clang
{
  class ASTConsumer;
  class CompilerInstance;
}

namespace cppbind
{

// Generic base class template providing some functionality around a
// FrontendAction instance. 'CONSUMER' should be a type inheriting from
// ASTConsumer.
template<typename CONSUMER>
class GenericFrontendAction : public clang::ASTFrontendAction
{
  using Include = std::pair<std::string, bool>;
  using Definition = std::pair<std::string, std::string>;

  // Preprocessor callbacks that extract include directives and macro definitions
  class PPCallback : public clang::PPCallbacks
  {
  public:
    PPCallback(std::deque<Include> *Includes,
               std::deque<Definition> *Definitions)
    : Includes_(Includes),
      Definitions_(Definitions)
    {}

    // This will extract all include directives in the temporary input file,
    // These include directives might e.g. originate in files containing
    // explicit template instantiations provided by the user and need to be
    // transferred to the generated source code files to guarantee correctness.
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

      Includes_->emplace_back(FileEntry->getName().str(), IsAngled);
    }

    // Extract macros defined in the original input file. This only considers
    // macros that look like they're defining constants, e.g. L4_PAGESHIT.
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

      // Only consider macros made up of numbers and arithmetic/logical operators.
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

    createPPCallback(CI);

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

  // Inheriting classes must implement this function that should return an
  // ASTConsumer instance which does all the 'real work', i.e. matches the AST
  // and executes appropriate callbacks.
  virtual std::unique_ptr<CONSUMER> makeConsumer() = 0;

  // Called before every translation unit is processed.
  virtual void beforeProcessing() {};

  // Called after every translation unit is processed.
  virtual void afterProcessing() {};

protected:
  std::deque<Include> const &includes() const
  { return Includes_; }

  std::deque<Definition> const &definitions() const
  { return Definitions_; }

private:
  void createPPCallback(clang::CompilerInstance &CI)
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
