#ifndef GUARD_CPPBIND_C_UTIL_INC
#define GUARD_CPPBIND_C_UTIL_INC

#include "cppbind/type_info.hpp"

extern "C" {

#include "cppbind/c/c_util.h"

void *_own(void *ptr)
{ return cppbind::type_info::_own(ptr); }

void *_disown(void *ptr)
{ return cppbind::type_info::_disown(ptr); }

void *_copy(void *ptr)
{ return cppbind::type_info::_copy(ptr); }

void *_move(void *ptr)
{ return cppbind::type_info::_move(ptr); }

void _delete(void *ptr)
{ cppbind::type_info::_delete(ptr); }

} // extern "C"

#endif // GUARD_CPPBIND_C_UTIL_INC
