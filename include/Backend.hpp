#ifndef GUARD_BACKEND_H
#define GUARD_BACKEND_H

#include <string>
#include <vector>

namespace cppbind
{

class WrapperFunction;
class WrapperRecord;

class Backend
{
public:
  static void run(std::vector<WrapperRecord> const &Records,
                  std::vector<WrapperFunction> const &Functions);

private:
  static auto addModuleSearchPath(std::string const &Path);
  static auto importModule(std::string const &Module);
};

} // namespace cppbind

#endif // GUARD_BACKEND_H
