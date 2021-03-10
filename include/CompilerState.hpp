#ifndef GUARD_COMPILER_STATE_H
#define GUARD_COMPILER_STATE_H

#include <cassert>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

#include "clang/AST/ASTContext.h"
#include "clang/Frontend/CompilerInstance.h"

#include "Mixin.hpp"

namespace cppbind
{

class CompilerStateRegistry : private mixin::NotCopyOrMoveable
{
  friend CompilerStateRegistry &CompilerState();

public:
  template<typename IT>
  void updateFileList(IT First, IT Last)
  {
    for (auto It = First; It != Last; ++It)
      updateFileEntry(*It);
  }

  void updateFile(std::string const &File);

  void updateCompilerInstance(clang::CompilerInstance const &CI)
  { CI_ = CI; }

  std::string currentFile(bool Relative = false) const;

  clang::CompilerInstance const &currentCompilerInstance() const
  {
    assert(CI_);
    return CI_->get();
  }

  clang::CompilerInstance const &operator*() const
  {
    assert(CI_);
    return CI_->get();
  }

  clang::CompilerInstance const *operator->() const
  {
    assert(CI_);
    return &CI_->get();
  }

private:
  CompilerStateRegistry() = default;

  static CompilerStateRegistry &instance()
  {
    static CompilerStateRegistry CS;

    return CS;
  }

  void updateFileEntry(std::string const &File);

  std::unordered_map<std::string, std::string> FilesByStem_;

  std::optional<std::string> File_;
  std::optional<std::reference_wrapper<clang::CompilerInstance const>> CI_;
};

inline CompilerStateRegistry &CompilerState()
{ return CompilerStateRegistry::instance(); }

inline clang::ASTContext &ASTContext()
{ return CompilerState()->getASTContext(); }

} // namespace cppbind

#endif // GUARD_COMPILER_STATE_H
