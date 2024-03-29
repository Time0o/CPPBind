#ifndef GUARD_TMP_FILE_H
#define GUARD_TMP_FILE_H

#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>

#include "Logging.hpp"
#include "Mixin.hpp"

namespace cppbind
{

// Abstracts a temporary file that is automatically deleted when not needed
// anymore.
class TmpFile : private mixin::NotCopyable
{
  template<typename T>
  friend TmpFile &operator<<(TmpFile &, T const &);

public:
  TmpFile();

  explicit TmpFile(std::string const &Path);

  explicit TmpFile(std::filesystem::path const &Path)
  : TmpFile(Path.string())
  {}

  ~TmpFile();

  TmpFile(TmpFile &&Other);
  void operator=(TmpFile &&Other);

  std::string path(bool Relative = false) const;

private:
  std::string Path_;
  bool Empty_ = true;
  bool Removed_ = false;
};

template<typename T>
TmpFile &operator<<(TmpFile &F, T const &What)
{
  std::ios_base::openmode Mode;

  if (F.Empty_) {
    Mode = std::fstream::out;
    F.Empty_ = false;
  } else {
    Mode = std::fstream::out | std::fstream::app;
  }

  std::ofstream FStream(F.Path_, Mode);

  if (!(FStream << What))
    throw log::exception("Failed to write to temporary file");

  FStream.flush();

  return F;
}

} // namespace cppbind

#endif // GUARD_TMP_FILE_H
