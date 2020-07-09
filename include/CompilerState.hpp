#ifndef GUARD_COMPILER_STATE_H
#define GUARD_COMPILER_STATE_H

#include <cassert>
#include <functional>
#include <optional>
#include <string>

#include "clang/Frontend/CompilerInstance.h"

namespace cppbind
{

class CompilerStateRegistry
{
  friend CompilerStateRegistry &CompilerState();

public:
  void updateFile(std::string const &File)
  { _File = File; }

  void updateCompilerInstance(clang::CompilerInstance const &CI)
  { _CI = CI; }

  std::string currentFile() const
  {
    assert(_File);
    return *_File;
  }

  clang::CompilerInstance const &currentCompilerInstance() const
  {
    assert(_CI);
    return _CI->get();
  }

  clang::CompilerInstance const &operator*() const
  {
    assert(_CI);
    return _CI->get();
  }

  clang::CompilerInstance const *operator->() const
  {
    assert(_CI);
    return &_CI->get();
  }

private:
  static CompilerStateRegistry &instance()
  {
    static CompilerStateRegistry CS;

    return CS;
  }

  std::optional<std::string> _File;
  std::optional<std::reference_wrapper<clang::CompilerInstance const>> _CI;
};

inline CompilerStateRegistry &CompilerState()
{ return CompilerStateRegistry::instance(); }

} // namespace cppbind

#endif // GUARD_COMPILER_STATE_H
