#ifndef GUARD_LOGGING_H
#define GUARD_LOGGING_H

#include <array>
#include <iostream>
#include <sstream>
#include <string>

namespace cppbind
{

namespace log
{

class CPPBindLogger
{
  static constexpr std::array<char const *, 4> Headers_ =
    { "debug ", "info", "warning", "error" };

public:
  enum Level { DEBUG = 1, INFO = 2, WARNING = 3, ERROR = 4 };

  explicit CPPBindLogger(Level Lvl,
                         std::ostream &Stdout = std::cout,
                         std::ostream &Stderr = std::cerr)
  : Lvl_(Lvl),
    Stdout_(Stdout),
    Stderr_(Stderr)
  { Buf_ << Headers_[Lvl_ - 1] << ": "; }

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
  std::ostringstream Buf_;
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
