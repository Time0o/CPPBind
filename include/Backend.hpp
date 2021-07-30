#ifndef GUARD_BACKEND_H
#define GUARD_BACKEND_H

#include <memory>
#include <string>

namespace cppbind
{

class Wrapper;

namespace backend
{

// Run the backend code for the current input file and the 'Wrapper*' objects
// created by CPPBind. Each invocation of 'run' independently executes the
// Python interpreter, i.e. there is no state preserved across runs for
// different translation units on the Python side. The 'Wrapper*' objects are
// exposed to Python via corresponding Python objects created with the help of
// pybind11 (see source/Backend.cpp).
void run(std::string const &InputFile, std::shared_ptr<Wrapper> Wrapper);

}

} // namespace cppbind

#endif // GUARD_BACKEND_H
