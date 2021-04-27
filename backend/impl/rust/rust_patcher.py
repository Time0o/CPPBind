from backend import backend
from cppbind import Constant, EnumConstant, Function, Identifier as Id, Type
from patcher import Patcher, _name
from text import code
from util import dotdict


def _type_target_c(self, scoped=True):
    args = dotdict({ 'scoped': scoped })

    target = backend().type_translator().target_c(self, args)

    return target


def _type_target(self):
    return backend().type_translator().target(self)


def _constant_declare(self):
    return f"pub static {self.name_target()}: {self.type().unqualified().target_c(scoped=False)};"


def _constant_assign(self):
    return code(
        f"""
        pub unsafe fn get_{self.name_target(case=Id.SNAKE_CASE)}() -> {self.type().unqualified().target()} {{
            c::{self.name_target()}
        }}
        """)


def _function_declare_parameters(self):
    return '\n'.join(f"let {p.name_interm()}: {p.type().target_c()};"
                     for p in self.parameters())


def _function_forward_parameters(self):
    translate_parameters = [
        p.type().input(inp=p.name_target(), interm=p.name_interm())
        for p in self.parameters()
    ]

    return '\n'.join(translate_parameters)


def _function_forward_call(self):
    parameters = ', '.join(p.name_interm() for p in self.parameters())

    call = f"c::{self.name_target()}({parameters})"

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
        self._patch(Constant, 'declare', _constant_declare)
        self._patch(Constant, 'assign', _constant_assign)
        self._patch(EnumConstant, 'name_target', _name(default_case=Id.PASCAL_CASE,
                                                       default_quals=Id.REMOVE_QUALS))
        self._patch(Function, 'declare_parameters', _function_declare_parameters)
        self._patch(Function, 'forward_call', _function_forward_call)
        self._patch(Function, 'forward', _function_forward)
        self._patch(Type, 'target_c', _type_target_c)
        self._patch(Type, 'target', _type_target)
        self._patch(Type, 'target', _type_target)
