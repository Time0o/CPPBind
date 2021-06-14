import c_util
from backend import Backend
from c_patcher import CPatcher
from c_type_translator import CTypeTranslator
from cppbind import Identifier as Id, Options
from text import code


class CBackend(Backend('c')):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self._patcher = CPatcher()
        self._type_translator = CTypeTranslator()

    def patcher(self):
        return self._patcher

    def type_translator(self):
        return self._type_translator

    def wrap_before(self):
        self._wrapper_header = self.output_file(
            self.input_file().modified(filename='{filename}_c', ext='c-header'))

        self._wrapper_source = self.output_file(
            self.input_file().modified(filename='{filename}_c', ext='cpp-source'))

        self._wrapper_header.append(code(
            """
            #ifndef {header_guard}
            #define {header_guard}

            #ifdef __cplusplus
            extern "C" {{
            #endif

            #include "cppbind/c/c_bind_error_c.h"

            {typedefs}

            {record_declarations}

            {record_definitions}
            """,
            header_guard=self._header_guard(),
            typedefs='\n'.join(self._typedefs()),
            record_declarations='\n'.join(self._record_declarations()),
            record_definitions='\n\n'.join(self._record_definitions('header'))))

        self._wrapper_source.append(code(
            """
            #include <cassert>
            #include <cerrno>
            #include <exception>
            #include <stdexcept>
            #include <utility>

            #include "cppbind/c/c_bind_error_cc.h"
            #include "cppbind/c/c_util_cc.h"

            {input_includes}

            extern "C" {{

            {input_header_include}

            {record_definitions}
            """,
            input_includes='\n'.join(self.input_includes()),
            input_header_include=self._wrapper_header.include(),
            record_definitions='\n\n'.join(self._record_definitions('source'))))

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

    def wrap_enum(self, e):
        enum_constants = [f"{c.name_target()} = {c.value(as_c_literal=True)}"
                          for c in e.constants()]

        enum_definition = code(
            """
            enum {enum_name} {{
              {enum_constants}
            }};
            """,
            enum_name=e.name_target(),
            enum_constants=',\n'.join(enum_constants))

        self._wrapper_header.append(enum_definition)

    def wrap_variable(self, v):
        self.wrap_function(v.getter())

        if v.is_assignable():
            self.wrap_function(v.setter())

    def wrap_function(self, f):
        self._wrapper_header.append(self._function_declaration(f))
        self._wrapper_source.append(self._function_definition(f))

    def wrap_record(self, r):
        for f in r.functions():
            self.wrap_function(f)

    def _header_guard(self):
        guard_id = self.input_file().filename().upper()

        return f"GUARD_{guard_id}_C_H"

    def _typedefs(self):
        typedefs = []
        for a, t in self.type_aliases():
            a_target = a.target()
            t_target = t.target()

            if a_target != t_target:
                typedefs.append(f"typedef {t_target} {a_target};")

        return typedefs

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
        # name
        name = f.name_target()

        # return type
        return_type = f.return_type()

        if return_type.is_record_indirection():
            return_type = return_type.pointee().without_const()

        return_type = return_type.target()

        # parameters
        parameters = []

        for p in f.parameters():
            t = p.type()
            if t.is_record():
                if t.proxy_for() is not None:
                    t = t.proxy_for()
                else:
                    t = t.with_const().pointer_to()

            parameters.append(f"{t.target()} {p.name_target()}")

        if not parameters:
            parameters = "void"
        else:
            parameters = ', '.join(parameters)

        # assemble
        return f"{return_type} {name}({parameters})"

    def _function_body(self, f):
        return f.forward()

    def _record_declarations(self):
        return [self._record_declaration(t) for t in self._record_types()]

    def _record_definitions(self, which):
        return [self._record_definition(t) for t in self._record_types(which)]

    def _record_declaration(self, t):
        return f"{t.target()};"

    def _record_definition(self, t):
        if t.as_record().is_abstract():
            union = code(
                """
                union { void *ptr; } obj;
                """)
        else:
            union = code(
                f"""
                union {{
                  char mem[{t.size()}];
                  void *ptr;
                }} obj;
                """)

        return code(
            """
            {name}
            {{
              {union}

              char is_initialized;
              char is_const;
              char is_owning;
            }};
            """,
            name=t.target(),
            union=union)

    def _record_types(self, which='all'):
        types_all = set()
        types_header = set()

        def add_record_type(t, which):
            if t.target().startswith('struct'):
                if which == 'all':
                    types_all.add(t)
                elif which == 'header':
                    types_header.add(t)

        for t in self._types:
            if t.is_record():
                add_record_type(t.without_const(), 'all')
            elif t.is_record_indirection():
                add_record_type(t.pointee().without_const(), 'all')

        for r in self.records():
            add_record_type(r.type(), 'header')

        types_source = types_all - types_header

        if which == 'all':
            types = types_all
        elif which == 'header':
            types = types_header
        elif which == 'source':
            types = types_source

        return list(sorted(types, key=lambda t: str(t)))

