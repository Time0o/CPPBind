from text import code
from type_info import TypeInfo as TI


class CUtil:
    @staticmethod
    def code_declare():
        return code(
            """
            void *_own(void *ptr);
            void *_disown(void *ptr);
            void *_copy(void *ptr);
            void *_move(void *ptr);
            void _delete(void *ptr);
            """)

    @staticmethod
    def code_define():
        return code(
            f"""
            void *_own(void *ptr)
            {{ return {TI.own('ptr')}; }}

            void *_disown(void *ptr)
            {{ return {TI.disown('ptr')}; }}

            void *_copy(void *ptr)
            {{ return {TI.copy('ptr')}; }}

            void *_move(void *ptr)
            {{ return {TI.move('ptr')}; }}

            void _delete(void *ptr)
            {{ {TI.delete('ptr')}; }}
            """)
