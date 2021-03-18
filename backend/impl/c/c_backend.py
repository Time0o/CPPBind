import c_patch
import c_type_translator
import c_util
from backend import Backend
from cppbind import Identifier as Id, Options
from text import code


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
            #include <cassert>
            #include <cerrno>
            #include <exception>
            #include <utility>

            {input_includes}

            namespace {{

            template<typename T, typename S>
            T *__struct_cast(S *s) {{
              if (s->is_owning)
                return reinterpret_cast<T *>(&s->obj.mem);
              else
                return static_cast<T *>(s->obj.ptr);
            }}

            template<typename S, typename T>
            T const *__struct_cast(S const *s) {{
              if (s->is_owning)
                return reinterpret_cast<T const *>(&s->obj.mem);
              else
                return static_cast<T const *>(s->obj.ptr);
            }}

            }} // namespace

            extern "C" {{

            {wrapper_header_include}
            """,
            input_includes='\n'.join(self.input_includes()),
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
        self._wrapper_header.append(self._record_definition(r))

        for f in r.functions():
            self.wrap_function(f)

    def wrap_function(self, f):
        self._wrapper_header.append(self._function_declaration(f))
        self._wrapper_source.append(self._function_definition(f))

    def _header_guard(self):
        guard_id = self.input_file().filename().upper()

        return f"GUARD_{guard_id}_C_H"

    def _record_definition(self, r):
        return code(
            f"""
            {r.type().c_struct()}
            {{
              union {{
                char mem[{r.type().size()}];
                void *ptr;
              }} obj;

              int is_initialized;
              int is_const;
              int is_owning;
            }};
            """)

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
        name = f.name_target()

        return_type = f.return_type().target()

        parameters = [
            f"{p.type().target()} {p.name_target()}"
            for p in f.parameters()
        ]

        out_type = None

        if f.return_type().is_record():
            out_type = f.return_type()
        elif f.return_type().is_pointer() and f.return_type().pointee().is_record():
            out_type = f.return_type().pointee()
        elif f.return_type().is_reference() and f.return_type().referenced().is_record():
            out_type = f.return_type().referenced()

        f.out_type = lambda: out_type

        if f.out_type() is not None:
            return_type = "void"
            parameters.append(f"{out_type.without_const().pointer_to().target()} {Id.OUT}")

        if not parameters:
            parameters = "void"
        else:
            parameters = ', '.join(parameters)

        return f"{return_type} {name}({parameters})"

    def _function_body(self, f):
        return f.forward()
