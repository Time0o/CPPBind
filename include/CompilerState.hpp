#ifndef GUARD_COMPILER_STATE_H
#define GUARD_COMPILER_STATE_H

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Frontend/CompilerInstance.h"

#include "IdentifierIndex.hpp"
#include "Mixin.hpp"
#include "TypeIndex.hpp"

namespace cppbind
{

enum InputFile
{
  ORIG_INPUT_FILE,
  TMP_INPUT_FILE,
  COMPLETE_INPUT_FILE
};

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

  void updateCompilerInstance(clang::CompilerInstance const &CI);

  std::string currentFile(InputFile IF, bool Relative = false) const;
  bool inCurrentFile(InputFile IF, clang::SourceLocation const &Loc) const;

  std::shared_ptr<IdentifierIndex> identifiers() const { return II_; }
  std::shared_ptr<TypeIndex> types() const { return TI_; }

  clang::CompilerInstance const &operator*() const;
  clang::CompilerInstance const *operator->() const;

private:
  CompilerStateRegistry() = default;

  static CompilerStateRegistry &instance()
  {
    static CompilerStateRegistry CS;

    return CS;
  }

  void updateFileEntry(std::string const &File);

  std::unordered_map<std::string, std::string> FilesByStem_;
  std::optional<std::string> TmpFile_;
  std::optional<std::string> File_;

  std::optional<std::reference_wrapper<clang::CompilerInstance const>> CI_;

  std::shared_ptr<IdentifierIndex> II_ = std::make_shared<IdentifierIndex>();
  std::shared_ptr<TypeIndex> TI_ = std::make_shared<TypeIndex>();
};

inline CompilerStateRegistry &CompilerState()
{ return CompilerStateRegistry::instance(); }

inline clang::ASTContext &ASTContext()
{ return CompilerState()->getASTContext(); }

} // namespace cppbind

#endif // GUARD_COMPILER_STATE_H
