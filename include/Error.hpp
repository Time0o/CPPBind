#ifndef GUARD_ERROR_H
#define GUARD_ERROR_H

#include <stdexcept>

namespace cppbind
{

class CPPBindError : public std::runtime_error
{
public:
  explicit CPPBindError()
  : std::runtime_error("cppbind error")
  {}
};

} // namespace cppbind

#endif // GUARD_ERROR_H
