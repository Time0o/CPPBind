#ifndef GUARD_LOGGING_H
#define GUARD_LOGGING_H

#include <exception>
#include <iostream>
#include <sstream>
#include <string>

namespace cppbind
{

namespace log
{

class CPPBindLogger
{
  static constexpr char const * _Headers[]
  { "debug ", "info", "warning", "error" };

public:
  enum Level { DEBUG = 1, INFO = 2, WARNING = 3, ERROR = 4 };

  explicit CPPBindLogger(Level Lvl,
                         std::ostream &Stdout = std::cout,
                         std::ostream &Stderr = std::cerr)
  : Lvl_(Lvl),
    Stdout_(Stdout),
    Stderr_(Stderr)
  { Buf_ << _Headers[Lvl_ - 1] << ": "; }

  ~CPPBindLogger()
  {
    auto Msg(Buf_.str());

    if (Lvl_ == ERROR) {
      Stderr_ << Msg << std::endl;
    } else if (Lvl_ == WARNING) {
      Stderr_ << Msg << std::endl;
    } else {
      Stdout_ << Msg << std::endl;
    }
  }

  template<typename T>
  CPPBindLogger &operator<<(T const &Value)
  {
    Buf_ << Value;
    return *this;
  }

private:
  int Lvl_;
  std::ostream &Stdout_, &Stderr_;
  std::stringstream Buf_;
};

inline CPPBindLogger debug()
{ return CPPBindLogger(CPPBindLogger::DEBUG); }

inline CPPBindLogger info()
{ return CPPBindLogger(CPPBindLogger::INFO); }

inline CPPBindLogger warn()
{ return CPPBindLogger(CPPBindLogger::WARNING); }

inline CPPBindLogger error()
{ return CPPBindLogger(CPPBindLogger::ERROR); }

} // namespace log

} // namespace cppbind

#endif // GUARD_LOGGING_H
