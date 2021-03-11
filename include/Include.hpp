#ifndef GUARD_INCLUDE_H
#define GUARD_INCLUDE_H

#include <string>

namespace cppbind
{

class Include
{
public:
  explicit Include(std::string const &Path, bool IsSystem = false);

  std::string path(bool Relative = false) const;

  std::string str(bool Relative = false) const;

private:
  std::string Path_;

  bool IsSystem_;
  bool IsAbsolute_;
};


} // namespace cppbind

#endif // GUARD_INCLUDE_H
