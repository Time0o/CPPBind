from backend import backend
from cppbind import Constant, EnumConstant, Function, Identifier as Id, Type
from patcher import Patcher, _name
from text import code
from util import dotdict


def _name_fallback_quals():
    return Id.REMOVE_QUALS


def _type_target_c(self, scoped=True):
    return backend().type_translator().target_c(self, dotdict({ 'scoped': scoped }))


def _type_target(self):
    return backend().type_translator().target(self)


def _constant_declare(self):
    c_name = self.name_target(quals=Id.REPLACE_QUALS)
    c_type = self.type().unqualified().target_c(scoped=False)

    return f"pub static {c_name}: {c_type};"


def _constant_assign(self):
    c_name = self.name_target(quals=Id.REPLACE_QUALS)

    rust_name = self.name_target(case=Id.SNAKE_CASE)
    rust_type = self.type().unqualified().target()

    return code(
        f"""
        pub unsafe fn get_{rust_name}() -> {rust_type} {{
            c::{c_name}
        }}
        """)


def _function_declare_parameters(self):
    decl = lambda p: f"let {p.name_interm()}: {p.type().target_c()};"

    return '\n'.join(decl(p) for p in self.parameters())


def _function_forward_parameters(self):
    transl = lambda p: p.type().input(inp=p.name_target(), interm=p.name_interm())

    return '\n'.join(transl(p) for p in self.parameters())


def _function_forward_call(self):
    c_name = self.name_target(quals=Id.REPLACE_QUALS)
    c_parameters = ', '.join(p.name_interm() for p in self.parameters())

    call = f"c::{c_name}({c_parameters})"

    return self.return_type().output(outp=call)


def _function_forward(self):
    forward = code(
        """
        {declare_parameters}

        {forward_parameters}

        {forward_call}
        """,
        declare_parameters=self.declare_parameters(),
        forward_parameters=self.forward_parameters(),
        forward_call=self.forward_call())

    return forward


class RustPatcher(Patcher):
    def patch(self):
        self._patch(_name, 'fallback_quals', _name_fallback_quals)
        self._patch(Constant, 'declare', _constant_declare)
        self._patch(Constant, 'assign', _constant_assign)
        self._patch(EnumConstant, 'name_target', _name(default_case=Id.PASCAL_CASE))
        self._patch(Function, 'declare_parameters', _function_declare_parameters)
        self._patch(Function, 'forward_call', _function_forward_call)
        self._patch(Function, 'forward', _function_forward)
        self._patch(Type, 'target_c', _type_target_c)
        self._patch(Type, 'target', _type_target)
        self._patch(Type, 'target', _type_target)
