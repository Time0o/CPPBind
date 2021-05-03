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
    def declare_parameter(p):
        decl_name = p.name_interm()
        decl_type = p.type()

        if decl_type.is_record():
            decl_type = decl_type.pointer_to()

        return f"let {decl_name}: {decl_type.target_c()};"

    return '\n'.join(declare_parameter(p) for p in self.parameters())


def _function_forward_parameters(self):
    transl = lambda p: p.type().input(inp=p.name_target(), interm=p.name_interm())

    return '\n'.join(transl(p) for p in self.parameters())


def _function_forward_call(self):
    c_name = self.name_target(quals=Id.REPLACE_QUALS)

    if self.is_copy_constructor():
        c_type = self.parent().type().with_const().pointer_to().target_c()

        call = f"c::{c_name}(self as {c_type})"

    elif self.is_move_constructor():
        pass # XXX

    else:
        c_parameters = ', '.join(p.name_interm() for p in self.parameters())

        call = f"c::{c_name}({c_parameters})"

    call = f"{self.return_type().output(outp=call)};"

    if not self.return_type().is_void():
        call = f"let {Id.RET} = {call}"

    if not self.is_noexcept():
        call = self.try_catch(call)

    return call


def _function_perform_return(self):
    if not self.return_type().is_void():
        return f"{Id.RET}" if self.is_noexcept() else f"Ok({Id.RET})"


def _function_try_catch(self, what):
    return code(
        """
        c::bind_error_reset();

        {what}

        if !c::bind_error_what().is_null() {{
            {handle_exception}
        }}
        """,
        what=what,
        handle_exception=self.handle_exception("c::bind_error_what()"))


def _function_handle_exception(self, what):
    return f"return Err(BindError::new({what}));"


def _function_forward(self):
    if self.is_copy_constructor() or self.is_move_constructor():
        return self.forward_call()

    if self.parameters():
        return code(
            """
            {declare_parameters}

            {forward_parameters}

            {forward_call}

            {perform_return}
            """,
            declare_parameters=self.declare_parameters(),
            forward_parameters=self.forward_parameters(),
            forward_call=self.forward_call(),
            perform_return=self.perform_return())
    else:
        return code(
            """
            {forward_call}

            {perform_return}
            """,
            forward_call=self.forward_call(),
            perform_return=self.perform_return())


class RustPatcher(Patcher):
    def patch(self):
        self._patch(_name, 'fallback_quals', _name_fallback_quals)
        self._patch(Constant, 'declare', _constant_declare)
        self._patch(Constant, 'assign', _constant_assign)
        self._patch(EnumConstant, 'name_target', _name(default_case=Id.PASCAL_CASE))
        self._patch(Function, 'declare_parameters', _function_declare_parameters)
        self._patch(Function, 'forward_call', _function_forward_call)
        self._patch(Function, 'try_catch', _function_try_catch)
        self._patch(Function, 'handle_exception', _function_handle_exception)
        self._patch(Function, 'perform_return', _function_perform_return)
        self._patch(Function, 'forward', _function_forward)
        self._patch(Type, 'target_c', _type_target_c)
        self._patch(Type, 'target', _type_target)
        self._patch(Type, 'target', _type_target)
