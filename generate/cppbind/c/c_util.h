#ifndef GUARD_CPPBIND_C_UTIL_H
#define GUARD_CPPBIND_C_UTIL_H

void *bind_own(void *ptr);
void *bind_disown(void *ptr);
void *bind_copy(void *ptr);
void *bind_move(void *ptr);
void bind_delete(void *ptr);

#define EBIND -1

#endif // GUARD_CPPBIND_C_UTIL_H
