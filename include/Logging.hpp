#ifndef GUARD_LOGGING_H
#define GUARD_LOGGING_H

#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace cppbind
{

class CPPBindError : public std::runtime_error
{
public:
  CPPBindError(std::string const &Msg)
  : std::runtime_error(Msg)
  {}
};

class CPPBindLogger
{
  static constexpr char const * _Headers[]
  { "debug ", "info", "warning", "error" };

public:
  enum Level { DEBUG = 1, INFO = 2, WARNING = 3, ERROR = 4 };

  CPPBindLogger(Level Lvl,
         std::ostream &Stdout = std::cout,
         std::ostream &Stderr = std::cerr)
  : _Lvl(Lvl),
    _Stdout(Stdout),
    _Stderr(Stderr)
  { _Buf << _Headers[_Lvl - 1] << ": "; }

  ~CPPBindLogger() noexcept(false)
  {
    auto Msg(_Buf.str());

    if (_Lvl == ERROR) {
      if (std::uncaught_exceptions() == 0)
        throw CPPBindError(Msg);
      else
        _Stderr << Msg << std::endl;
    } else if (_Lvl == WARNING) {
      _Stderr << Msg << std::endl;
    } else {
      _Stdout << Msg << std::endl;
    }
  }

  template<typename T>
  CPPBindLogger &operator<<(T const &Value)
  {
    _Buf << Value;
    return *this;
  }

private:
  int _Lvl;
  std::ostream &_Stdout, &_Stderr;
  std::stringstream _Buf;
};

inline CPPBindLogger debug()
{ return CPPBindLogger(CPPBindLogger::DEBUG); }

inline CPPBindLogger info()
{ return CPPBindLogger(CPPBindLogger::INFO); }

inline CPPBindLogger warn()
{ return CPPBindLogger(CPPBindLogger::WARNING); }

inline CPPBindLogger error()
{ return CPPBindLogger(CPPBindLogger::ERROR); }

} // namespace cppbind

#endif // GUARD_LOGGING_H
