from enum import Enum

from backend import BackendBase
from cppbind import Identifier as Id, Type, Variable, Record, Function, Parameter
from type_translator import TypeTranslator; TT = TypeTranslator()
from text import code
from util import dotdict

Type.__str__ = lambda self: self.str()

import text


def _name_c(get=lambda self: self.name, case=Id.SNAKE_CASE):
    @property
    def _name_c_closure(self):
        return get(self).format(case=case, quals=Id.REPLACE_QUALS)

    return _name_c_closure

Variable.name_c = _name_c(case=Id.SNAKE_CASE_CAP_ALL)
Function.name_c = _name_c(get=lambda self: self.name_overloaded)
Parameter.name_c = _name_c()


class CBackend(BackendBase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        input_file = self.input_file()

        self._wrapper_header = self.output_file(
            input_file.modified(filename='{filename}_c', ext='.h'))

        self._wrapper_source = self.output_file(
            input_file.modified(filename='{filename}_c', ext='.cpp'))

    def wrap_before(self):
        self._wrapper_header.append(code(
            """
            #ifndef {header_guard}
            #define {header_guard}

            #ifdef __cplusplus
            extern "C" {{
            #endif

            #include <stdbool.h>
            """,
            header_guard=self._header_guard()))

        self._wrapper_source.append(code(
            """
            #include <utility>

            {input_header_include}

            {type_info}

            extern "C" {{

            {wrapper_header_include}
            """,
            input_header_include=self.input_file().include(),
            type_info=self.type_info().code(),
            wrapper_header_include=self._wrapper_header.include()))

    def wrap_after(self):
        self._wrapper_header.append(code(
            """
            #ifdef __cplusplus
            }} // extern "C"
            #endif

            #endif // {header_guard}
            """,
            header_guard=self._header_guard()))

        self._wrapper_source.append(code(
            """
            } // extern "C"
            """))

    def wrap_variable(self, v):
        #XXX generalize
        self._wrapper_header.append(f"extern {TT.c(v.type)} {v.name_c};")

        args = dotdict({
            'v': v
        })

        self._wrapper_source.append(
            TT.variable(v.type, args).format(varin=v.name,
                                             varout=f"{TT.c(v.type)} {v.name_c}"))

    def wrap_record(self, r):
        for f in r.functions:
            self.wrap_function(f)

    def wrap_function(self, f):
        if f.is_constructor():
            f.return_type = f.parent.type.pointer_to()

        self._wrapper_header.append(self._function_declaration(f))
        self._wrapper_source.append(self._function_definition(f))

    def _header_guard(self):
        guard_id = self.input_file().filename().upper()

        return f"GUARD_{guard_id}_C_H"

    def _function_declaration(self, f):
        header = self._function_header(f)

        return code(f"{header};")

    def _function_definition(self, f):
        return code(
            """
            {header}
            {{
              {body}
            }}
            """,
            header=self._function_header(f),
            body=self._function_body(f))

    def _function_header(self, f):
        #XXX generalize
        parameters = ', '.join(f"{TT.c(p.type)} {p.name_c}" for p in f.parameters)

        return f"{TT.c(f.return_type)} {f.name_c}({parameters})"

    def _function_body(self, f):
        return code(
            f"""
            {{declare_parameters}}

            {{forward_parameters}}

            {{forward_call}}
            """,
            declare_parameters=f.declare_parameters(),
            forward_parameters=f.forward_parameters(),
            forward_call=f.forward_call())
