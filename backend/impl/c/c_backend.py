import c_util
import type_info

from backend import Backend
from cppbind import Identifier as Id, Type, Constant, Record, Function, Parameter
from type_translator import TypeTranslator as TT
from text import code
from util import dotdict


class CBackend(Backend):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        input_file = self.input_file()

        self._wrapper_header = self.output_file(
            input_file.modified(filename='{filename}_c',
                                ext=self.option('output-c-header-extension')))

        self._wrapper_source = self.output_file(
            input_file.modified(filename='{filename}_c',
                                ext=self.option('output-cpp-source-extension')))

    def wrap_before(self):
        self._wrapper_header.append(code(
            """
            #ifndef {header_guard}
            #define {header_guard}

            #ifdef __cplusplus
            extern "C" {{
            #endif

            #include <stdbool.h>

            {c_util_include}
            """,
            header_guard=self._header_guard(),
            c_util_include=c_util.include()))

        self._wrapper_source.append(code(
            """
            #include <utility>

            {input_header_include}

            {type_info_include}

            {type_info_type_instances}

            extern "C" {{

            {wrapper_header_include}

            {c_util_include}
            """,
            input_header_include=self.input_file().include(),
            type_info_include=type_info.include(),
            type_info_type_instances=type_info.type_instances(),
            wrapper_header_include=self._wrapper_header.include(),
            c_util_include=c_util.include(impl=True)))

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

    def wrap_constant(self, v):
        #XXX generalize
        self._wrapper_header.append(f"extern {TT().c(v.type())} {v.name_target()};")

        args = dotdict({
            'v': v
        })

        self._wrapper_source.append(
            TT().constant(v.type(), args).format(
                varin=v.name(), varout=f"{TT().c(v.type())} {v.name_target()}"))

    def wrap_record(self, r):
        for f in r.functions():
            if not f.is_destructor():
                self.wrap_function(f)

    def wrap_function(self, f):
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
        parameters = ', '.join(f"{TT().c(p.type())} {p.name_target()}"
                               for p in f.parameters())

        return f"{TT().c(f.return_type())} {f.name_target()}({parameters})"

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
