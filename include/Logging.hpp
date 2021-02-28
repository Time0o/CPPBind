#ifndef GUARD_LOGGING_H
#define GUARD_LOGGING_H

#include <stdexcept>
#include <string>
#include <utility>

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

template<typename ...ARGS>
std::string fmt(std::string Fmt, ARGS &&...Args)
{
  if (Fmt.empty()) {
    for (std::size_t i = 0; i < sizeof...(Args); ++i)
      Fmt += llvm::formatv("{{}} ", i);
  }

  return llvm::formatv(Fmt.c_str(), std::forward<ARGS>(Args)...);
}

template<typename ...ARGS>
void log(llvm::raw_ostream &S, std::string const &Fmt, ARGS &&...Args)
{ S << fmt(Fmt, std::forward<ARGS>(Args)...) << '\n'; }

template<typename ...ARGS>
void info(std::string const &Fmt, ARGS &&...Args)
{ log(llvm::outs(), "INFO: " + Fmt, std::forward<ARGS>(Args)...); }

template<typename ...ARGS>
void warning(std::string const &Fmt, ARGS &&...Args)
{ log(llvm::outs(), "WARNING: " + Fmt, std::forward<ARGS>(Args)...); }

template<typename ...ARGS>
void error(std::string const &Fmt, ARGS &&...Args)
{ log(llvm::errs(), "ERROR: " + Fmt, std::forward<ARGS>(Args)...); }

template<typename ...ARGS>
void exception(std::string const &Fmt, ARGS &&...Args)
{ throw std::runtime_error(fmt(Fmt, std::forward<ARGS>(Args)...)); }

#endif // GUARD_LOGGING_H
