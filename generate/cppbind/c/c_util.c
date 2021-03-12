#ifndef GUARD_CPPBIND_C_UTIL_INC
#define GUARD_CPPBIND_C_UTIL_INC

#include "cppbind/type_info.hpp"

extern "C" {

#include "cppbind/c/c_util.h"

void *bind_own(void *ptr)
{ return cppbind::type_info::bind_own(ptr); }

void *bind_disown(void *ptr)
{ return cppbind::type_info::bind_disown(ptr); }

void *bind_copy(void *ptr)
{ return cppbind::type_info::bind_copy(ptr); }

void *bind_move(void *ptr)
{ return cppbind::type_info::bind_move(ptr); }

void bind_delete(void *ptr)
{ cppbind::type_info::bind_delete(ptr); }

} // extern "C"

#endif // GUARD_CPPBIND_C_UTIL_INC
