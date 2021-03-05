#ifndef GUARD_CPPBIND_C_UTIL_H
#define GUARD_CPPBIND_C_UTIL_H

void *_own(void *ptr);
void *_disown(void *ptr);
void *_copy(void *ptr);
void *_move(void *ptr);
void _delete(void *ptr);

#endif // GUARD_CPPBIND_C_UTIL_H
