#ifndef GUARD_ENV_H
#define GUARD_ENV_H

#include <optional>
#include <string>
#include <unordered_map>

namespace cppbind {

class EnvRegistry
{
  friend EnvRegistry &Env();

public:
  std::optional<std::string> get(std::string const &Key) const
  {
    auto It(Env_.find(Key));
    if (It == Env_.end())
      return std::nullopt;

    return It->second;
  }

  void set(std::string const &Key, std::string const &Val)
  { Env_[Key] = Val; }

private:
  EnvRegistry() = default;

  static EnvRegistry &instance()
  {
    static EnvRegistry Env;
    return Env;
  }

  std::unordered_map<std::string, std::string> Env_;
};

inline EnvRegistry &Env()
{ return EnvRegistry::instance(); }

} // namespace cppbind

#endif // GUARD_ENV_H
