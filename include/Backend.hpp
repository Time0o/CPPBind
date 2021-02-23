#ifndef GUARD_BACKEND_H
#define GUARD_BACKEND_H

#include <memory>
#include <string>
#include <vector>

namespace cppbind
{

class Wrapper;

class Backend
{
public:
  static void run(std::string const &InputFile,
                  std::shared_ptr<Wrapper> Wrapper);

private:
  static auto addModuleSearchPath(std::string const &Path);
  static auto importModule(std::string const &Module);
};

} // namespace cppbind

#endif // GUARD_BACKEND_H
