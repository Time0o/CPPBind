#ifndef GUARD_WRAPPER_FILE_H
#define GUARD_WRAPPER_FILE_H

#include <filesystem>
#include <sstream>
#include <string>
#include <system_error>

#include "LLVMError.hpp"
#include "Options.hpp"
#include "String.hpp"

namespace cppbind
{

class FileBuffer
{
public:
  enum : char {
    EndLine = '\n',
    EmptyLine = '\n'
  };

  FileBuffer(std::filesystem::path const &Path)
  : _Path(Path)
  {}

  template<typename T>
  FileBuffer &operator<<(T const &Line)
  {
    _Content << Line;
    return *this;
  }

  void write() const
  {
    // open file
    std::error_code EC;

    llvm::raw_fd_ostream OS(_Path.c_str(), EC, llvm::sys::fs::F_None);

    if (EC)
      throw LLVMError("Failed to open '" + _Path.string() + "'", EC);

    // write to file
    OS.clear_error();

    auto Content(_Content.str());

    OS << trimStr(Content);

    EC = OS.error();

    if (EC)
      throw LLVMError("Failed to write '" + _Path.string() + "'", EC);
  }

private:
  std::stringstream _Content;
  std::filesystem::path _Path;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_FILE_H
