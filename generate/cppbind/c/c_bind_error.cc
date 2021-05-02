#include <string>

thread_local std::string error;

std::string *__bind_error_location()
{ return &error; }

extern "C" {

char const *bind_error_what()
{ return error.empty() ? nullptr : error.c_str(); }

void bind_error_reset()
{ return error.clear(); }

}
