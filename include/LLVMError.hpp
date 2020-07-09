#ifndef GUARD_LLVM_ERROR_H
#define GUARD_LLVM_ERROR_H

#include <stdexcept>
#include <string>
#include <system_error>

namespace cppbind
{

struct LLVMError : public std::runtime_error
{
  LLVMError(std::string const &Msg) noexcept
  : std::runtime_error(Msg)
  {}

  LLVMError(std::string const &Msg, std::error_code EC) noexcept
  : std::runtime_error(Msg + ": " + EC.message())
  {}
};

} // namespace cppbind

#endif // GUARD_LLVM_ERROR_H
