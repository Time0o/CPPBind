#ifndef GUARD_CPPBIND_C_BIND_ERROR_CC_H
#define GUARD_CPPBIND_C_BIND_ERROR_CC_H

#include <string>

std::string *__bind_error_location();

#define bind_error (*__bind_error_location())

#endif // GUARD_CPPBIND_C_BIND_ERROR_CC_H
