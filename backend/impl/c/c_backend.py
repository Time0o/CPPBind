from backend import *

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
        cpp_name = v.name()

        c_name = self._c_variable_name(v)
        c_type = self._c_type(v.type())

        self._wrapper_header.append(text.code(
            f"extern {c_type} {c_name};"))

        self._wrapper_source.append(text.code(
            f"{c_type} {c_name} = static_cast<{c_type}>({cpp_name});"))

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
        return_type = f.return_type()
        if return_type.is_lvalue_reference():
            return_type = return_type.referenced().pointer_to()

        c_name = self._c_function_name(f)
        c_return_type = self._c_type(return_type)
        parameters = self._function_parameter_declarations(f)

        return f"{c_return_type} {c_name}({parameters})"

    def _function_body(self, f):
        cpp_name = f.name()
        cpp_return_type = f.return_type()
        parameters = self._function_parameter_forwardings(f)

        body = f"{cpp_name}({parameters});"

        if cpp_return_type.is_lvalue_reference():
            body = '&' + body

        if not cpp_return_type.is_fundamental('void'):
            body = 'return ' + body

        return body

    def _function_parameter_declarations(self, f):
        def declaration(p):
            p_type = self._c_type(p.type())
            p_name = self._c_parameter_name(p)

            if p_type.is_lvalue_reference():
                p_type = p_type.referenced().pointer_to()
            elif p_type.is_rvalue_reference():
                p_type = p_type.referenced()

            return f"{p_type} {p_name}"

        return ', '.join(declaration(p)
                         for p in self._function_non_default_parameters(f))

    def _function_parameter_forwardings(self, f):
        def forwarding(p):
            p_type = self._c_type(p.type())
            p_forward = self._c_parameter_name(p)

            if p_type.is_lvalue_reference():
                p_forward = '*' + p_forward
            elif p_type.is_rvalue_reference():
                self._add_source_include('utility')
                p_forward = f"std::move({p_forward})"

            return f"{p_forward}"

        return ', '.join(forwarding(p)
                         for p in self._function_non_default_parameters(f))

    def _function_non_default_parameters(self, f):
        return [p for p in f.parameters() if not p.has_default_argument()]

    # XXX
    def _c_type(self, t):
        if not t.is_fundamental():
            return t

        c_header = self._c_header(t)
        if c_header is not None:
            self._add_header_include(c_header)

        return t

    # XXX
    def _c_header(self, t):
        if not t.is_fundamental():
            return None

        in_header = {
            'bool': 'stdbool',
            'wchar_t': 'wchar'
        }

        for k, v in in_header.items():
            if t.is_fundamental(k):
                return f'{v}.h'

    @staticmethod
    def _c_variable_name(v):
        return v.name(qualifiers='replace', case='snake-cap-all')

    @staticmethod
    def _c_function_name(f):
        return f.overload_name(qualifiers='replace', case='snake')

    @staticmethod
    def _c_parameter_name(p):
        return p.name(case='snake')
