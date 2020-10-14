#ifndef GUARD_WRAPPER_FILE_H
#define GUARD_WRAPPER_FILE_H

#include <filesystem>
#include <sstream>
#include <string>
#include <system_error>

#include "Logging.hpp"
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

  template<typename T>
  FileBuffer &operator<<(T const &Line)
  {
    Content_ << Line;
    return *this;
  }

  void write(std::filesystem::path const &Path) const
  {
    // open file
    std::error_code EC;

    llvm::raw_fd_ostream OS(Path.c_str(), EC, llvm::sys::fs::F_None);

    if (EC)
      error() << "Failed to open '" << Path.string() << "'" << EC.message();

    // write to file
    OS.clear_error();

    auto Content(Content_.str());

    OS << trimStr(Content);

    EC = OS.error();

    if (EC)
      error() << "Failed to write '" << Path.string() << "'" << EC.message();
  }

private:
  std::stringstream Content_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_FILE_H
