#ifndef GUARD_WRAPPER_FILE_H
#define GUARD_WRAPPER_FILE_H

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
    Empty_ = false;
    Content_ << Line;
    return *this;
  }

  bool empty() const
  { return Empty_; }

  std::string content() const
  { return Content_.str(); }

  void write(std::string const &Path) const
  {
    // open file
    std::error_code EC;

    llvm::raw_fd_ostream OS(Path.c_str(), EC, llvm::sys::fs::F_None);

    if (EC)
      error() << "Failed to open '" << Path << "'" << EC.message();

    // write to file
    OS.clear_error();

    auto Content(Content_.str());

    OS << trimStr(Content);

    EC = OS.error();

    if (EC)
      error() << "Failed to write '" << Path << "'" << EC.message();
  }

private:
  bool Empty_ = true;
  std::stringstream Content_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_FILE_H
