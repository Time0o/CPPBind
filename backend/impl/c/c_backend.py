import c_util
import type_info
from backend import Backend
from cppbind import Options
from text import code
from util import dotdict


class CBackend(Backend):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        input_file = self.input_file()

        self._wrapper_header = self.output_file(
            input_file.modified(filename='{filename}_c', ext='c-header'))

        self._wrapper_source = self.output_file(
            input_file.modified(filename='{filename}_c', ext='cpp-source'))

    def wrap_before(self):
        self._wrapper_header.append(code(
            """
            #ifndef {header_guard}
            #define {header_guard}

            #ifdef __cplusplus
            extern "C" {{
            #endif

            {c_util_include}
            """,
            header_guard=self._header_guard(),
            c_util_include=c_util.path().include()))

        self._wrapper_source.append(code(
            """
            #include <cerrno>
            #include <exception>
            #include <utility>

            {input_includes}

            {type_info_include}

            {type_info_type_instances}

            extern "C" {{

            {wrapper_header_include}
            """,
            input_includes='\n'.join(self.input_includes()),
            type_info_include=type_info.path().include(),
            type_info_type_instances=type_info.type_instances(),
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

    def wrap_constant(self, c):
        self._wrapper_header.append(f"extern {c.type().target()} {c.name_target()};")
        self._wrapper_source.append(c.assign())

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
        if not f.parameters():
            parameters = "void"
        else:
            parameters = ', '.join(f"{p.type().target()} {p.name_target()}"
                                   for p in f.parameters())

        return f"{f.return_type().target()} {f.name_target()}({parameters})"

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
