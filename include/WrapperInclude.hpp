#ifndef GUARD_WRAPPER_INCLUDE_H
#define GUARD_WRAPPER_INCLUDE_H

#include <ostream>
#include <string>

namespace cppbind
{

class WrapperInclude
{
public:
  WrapperInclude(std::string const &Header, bool System, bool Public = true)
  : Header_(Header),
    System_(System),
    Public_(Public)
  {}

  bool operator==(WrapperInclude const &Wi) const
  { return Header_ == Wi.Header_ && System_ == Wi.System_; }

  bool operator<(WrapperInclude const &Wi) const
  {
    if (System_ && !Wi.System_)
      return true;

    if (!System_ && Wi.System_)
      return false;

    return Header_ < Wi.Header_;
  }

  std::string header() const
  { return Header_; }

  bool isSystem() const
  { return System_; }

  bool isPublic() const
  { return Public_; }

private:
  std::string Header_;
  bool System_;
  bool Public_;
};

inline std::ostream &operator<<(std::ostream &Os, WrapperInclude const &Include)
{
  Os << (Include.isSystem() ? '<' : '"')
     << Include.header()
     << (Include.isSystem() ? '>' : '"');

  return Os;
}

} // namespace cppbind

#endif // WRAPPER_INCLUDE_H
