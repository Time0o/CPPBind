#ifndef GUARD_LOGGING_H
#define GUARD_LOGGING_H

#include <stdexcept>
#include <string>
#include <utility>

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

namespace cppbind
{

namespace log
{

template<typename ...ARGS>
std::string fmt(std::string Fmt, ARGS &&...Args)
{
  if (Fmt.empty()) {
    for (std::size_t i = 0; i < sizeof...(Args); ++i)
      Fmt += llvm::formatv("{{}} ", i);
  }

  return llvm::formatv(Fmt.c_str(), std::forward<ARGS>(Args)...);
}

inline int Verbosity = 0;

template<int VERBOSITY, typename ...ARGS>
void log(llvm::raw_ostream &S, std::string const &Fmt, ARGS &&...Args)
{
  if (Verbosity >= VERBOSITY)
    S << fmt(Fmt, std::forward<ARGS>(Args)...) << '\n';
}

template<typename ...ARGS>
void debug(std::string const &Fmt, ARGS &&...Args)
{ log<2>(llvm::outs(), "DEBUG: " + Fmt, std::forward<ARGS>(Args)...); }

template<typename ...ARGS>
void info(std::string const &Fmt, ARGS &&...Args)
{ log<1>(llvm::outs(), "INFO: " + Fmt, std::forward<ARGS>(Args)...); }

template<typename ...ARGS>
void warning(std::string const &Fmt, ARGS &&...Args)
{ log<0>(llvm::errs(), "WARNING: " + Fmt, std::forward<ARGS>(Args)...); }

template<typename ...ARGS>
void error(std::string const &Fmt, ARGS &&...Args)
{ log<0>(llvm::errs(), "ERROR: " + Fmt, std::forward<ARGS>(Args)...); }

template<typename ...ARGS>
std::runtime_error exception(std::string const &Fmt, ARGS &&...Args)
{ return std::runtime_error(fmt(Fmt, std::forward<ARGS>(Args)...)); }

} // namespace log

} // namespace cppbind

#endif // GUARD_LOGGING_H
