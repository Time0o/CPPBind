from text import code
from type_info import TypeInfo as TI


class CUtil:
    @staticmethod
    def code_declare():
        return "void c_delete(void *ptr);"

    @staticmethod
    def code_define():
        return code(
            f"""
            void c_delete(void *ptr)
            {{ delete {TI.get_typed('ptr')}; }}
            """)
