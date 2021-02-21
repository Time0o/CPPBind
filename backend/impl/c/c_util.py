import type_info as ti

from text import code


def declare():
    return code(
        """
        void *_own(void *ptr);
        void *_disown(void *ptr);
        void *_copy(void *ptr);
        void *_move(void *ptr);
        void _delete(void *ptr);
        """)


def define():
    return code(
        f"""
        void *_own(void *ptr)
        {{ return {ti.own('ptr')}; }}

        void *_disown(void *ptr)
        {{ return {ti.disown('ptr')}; }}

        void *_copy(void *ptr)
        {{ return {ti.copy('ptr')}; }}

        void *_move(void *ptr)
        {{ return {ti.move('ptr')}; }}

        void _delete(void *ptr)
        {{ {ti.delete('ptr')}; }}
        """)
