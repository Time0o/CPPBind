#include "boost/filesystem/path.hpp"

#include "CompilerState.hpp"

using namespace boost::filesystem;

namespace cppbind
{

void
CompilerStateRegistry::updateFile(std::string const &File)
{ File_ = path(File).filename().native(); }

} // namespace cppbind
