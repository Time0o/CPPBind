#ifndef GUARD_ERROR_H
#define GUARD_ERROR_H

#include <sstream>
#include <stdexcept>

namespace cppbind
{

class CPPBindError : public std::runtime_error
{
public:
  explicit CPPBindError(std::string const &Msg)
  : std::runtime_error(Msg)
  {}
};

} // namespace cppbind

#endif // GUARD_ERROR_H
