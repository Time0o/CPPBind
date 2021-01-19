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

class ErrorMsg
{
public:
    enum : char
    { endl = '\n' };

    template<typename T>
    ErrorMsg &operator<<(T const  &Val)
    {
      SS_ << Val;
      return *this;
    }

    operator std::string() const
    { return SS_.str(); }

private:
    std::stringstream SS_;
};

} // namespace cppbind

#endif // GUARD_ERROR_H
