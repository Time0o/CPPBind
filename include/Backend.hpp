#ifndef GUARD_BACKEND_H
#define GUARD_BACKEND_H

#include <memory>
#include <string>
#include <vector>

#include "pybind11/embed.h"

namespace cppbind
{

class Wrapper;

class Backend
{
public:
  static void run(std::string const &InputFile,
                  std::shared_ptr<Wrapper> Wrapper);

private:
  static pybind11::module_ importModule(std::string const &Module);

  static void addModuleSearchPath(pybind11::module_ const &Sys,
                                  std::string const &Path);
};

} // namespace cppbind

#endif // GUARD_BACKEND_H
