from backend import *
from cppbind import Identifier

import text


class CBackend(Backend):
    def __init__(self, *args):
        super().__init__(*args)

        input_file = self.input_file()

        self._wrapper_header = self.output_file(
            input_file.modified(filename='{filename}_c', ext='.h'))

        self._wrapper_source = self.output_file(
            input_file.modified(filename='{filename}_c', ext='.cpp'))

        self._header_include_set = set()
        self._source_include_set = set()

    def wrap_before(self):
        pass

    def wrap_after(self):
        self._wrapper_header.prepend(text.code(
            """
            #ifndef {header_guard}
            #define {header_guard}

            #ifdef __cplusplus
            extern "C" {{
            #endif

            {header_includes}
            """,
            header_guard=self._header_guard(),
            header_includes=self._header_includes()
        ))

        self._wrapper_header.append(text.code(
            """
            #ifdef __cplusplus
            }} // extern "C"
            #endif

            #endif // {header_guard}
            """,
            header_guard=self._header_guard()
        ))

        self._wrapper_source.prepend(text.code(
            """
            {source_includes}

            {input_include}

            extern "C" {{

            {wrapper_include}
            """,
            source_includes=self._source_includes(),
            input_include=text.include(self.input_file().basename()),
            wrapper_include=text.include(self._wrapper_header.basename()),
        ))

        self._wrapper_source.append(text.code(
            """
            } // extern "C"
            """
        ))

    def wrap_variable(self, v):
        c_name = self._c_variable_name(v)
        c_type = self._c_type(v.type)

        self._wrapper_header.append(text.code(
            f"extern {c_type} {c_name};"))

        self._wrapper_source.append(text.code(
            f"{c_type} {c_name} = static_cast<{c_type}>({v.name});"))

    def wrap_function(self, f):
        self._wrapper_header.append(self._function_declaration(f))
        self._wrapper_source.append(self._function_definition(f))

    def _header_includes(self):
        return '\n'.join(self._header_include_set)

    def _source_includes(self):
        return '\n'.join(self._source_include_set)

    def _add_header_include(self, header):
        self._header_include_set.add(text.include(header, system=True))

    def _add_source_include(self, header):
        self._source_include_set.add(text.include(header, system=True))

    def _header_guard(self):
        guard_id = self.input_file().filename().upper()

        return f"GUARD_{guard_id}_C_H"

    def _function_declaration(self, f):
        header = self._function_header(f)

        return text.code(f"{header};")

    def _function_definition(self, f):
        header = self._function_header(f)
        body = self._function_body(f)

        return text.code(
            f"""
            {header}
            {{ {body} }}
            """)

    def _function_header(self, f):
        c_name = self._c_function_name(f)
        c_return_type = self._c_type(f.return_type)
        c_parameters = self._function_parameter_declarations(f)

        return f"{c_return_type} {c_name}({c_parameters})"

    def _function_body(self, f):
        body = f"{f.name}({self._function_parameter_forwardings(f)})"

        if f.return_type.is_void():
            return f"{body};"

        if f.return_type.is_lvalue_reference():
            body = '&' + body

        body = self._enum_cast(f.return_type, body, remove=True)

        return f"return {body};";

    def _function_parameter_declarations(self, f):
        def declaration(p):
            c_type = self._c_type(p.type)
            c_name = self._c_parameter_name(p)

            return f"{c_type} {c_name}"

        return ', '.join(declaration(p) for p in f.parameters())

    def _function_parameter_forwardings(self, f):
        def forward(p):
            fwd = self._enum_cast(p.type, self._c_parameter_name(p), remove=False)

            if p.type.is_lvalue_reference():
                fwd = '*' + fwd
            elif p.type.is_rvalue_reference():
                self._add_source_include('utility')
                fwd = f"std::move({fwd})"

            return fwd

        return ', '.join(forward(p) for p in f.parameters())

    def _enum_cast(self, pt, what, remove):
        if not pt.base.is_scoped_enum():
            return what

        if pt.is_lvalue_reference() or pt.is_pointer():
            cast = 'reinterpret_cast'
        else:
            cast = 'static_cast'

        cast_to = self._c_type(pt, without_enum=remove)

        return f"{cast}<{cast_to}>({what})"

    # XXX
    def _c_type(self, t, without_enum=True):
        if without_enum:
            t = t.without_enum()

        if t.is_lvalue_reference():
            t = t.referenced().pointer_to()
        elif t.is_rvalue_reference():
            t = t.referenced()

        c_header = self._c_header(t)
        if c_header is not None:
            self._add_header_include(c_header)

        return t

    # XXX
    @staticmethod
    def _c_header(t):
        if not t.is_fundamental():
            return None

        in_header = {
            'bool': 'stdbool'
        }

        for k, v in in_header.items():
            if t.is_fundamental(k):
                return f'{v}.h'

    @staticmethod
    def _c_variable_name(v):
        return v.name.format(case=Identifier.SNAKE_CASE_CAP_ALL,
                             quals=Identifier.REPLACE_QUALS)

    @staticmethod
    def _c_function_name(f):
        return f.name_overloaded.format(case=Identifier.SNAKE_CASE,
                                        quals=Identifier.REPLACE_QUALS)

    @staticmethod
    def _c_parameter_name(p):
        return p.name.format(case=Identifier.SNAKE_CASE)
