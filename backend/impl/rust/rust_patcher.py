from backend import backend
from patcher import Patcher, _name
from pycppbind import EnumConstant, Function, Identifier as Id, Record, Type
from text import code
from util import dotdict


# XXX add this for other languages
def _id_reserved():
    return set({
        'Self', 'abstract', 'as', 'async', 'await', 'become', 'box', 'break',
        'const', 'continue', 'crate', 'do', 'dyn', 'else', 'enum', 'extern',
        'false', 'final', 'fn', 'for', 'if', 'impl', 'in', 'let', 'loop',
        'macro', 'match', 'mod', 'move', 'mut', 'override', 'priv', 'pub',
        'ref', 'return', 'self', 'struct', 'super', 'trait', 'true', 'try',
        'type', 'typeof', 'union', 'unsafe', 'unsized', 'use', 'virtual',
        'where', 'while', 'yield'
    })


def _type_target_c(self, scoped=True):
    return backend().type_translator().target_c(self, dotdict({ 'scoped': scoped }))


def _type_target(self):
    return backend().type_translator().target(self)


def _function_declare_parameters(self):
    def declare_parameter(p):
        t = p.type()

        if t.is_record():
            if t.proxy_for() is not None:
                t = t.proxy_for()
            else:
                t = t.with_const().pointer_to()

        return f"let {p.name_interm()}: {t.target_c()};"

    return '\n'.join(declare_parameter(p) for p in self.parameters())


def _function_forward_parameters(self):
    transl = lambda p: p.type().input(inp=p.name_target(), interm=p.name_interm())

    return '\n'.join(transl(p) for p in self.parameters())


def _function_forward_call(self):
    c_name = f"{backend()._c_lib()}::{self.name_target()}"

    if self.is_copy_constructor():
        c_type_self = self.parent().type().with_const().pointer_to().target_c()

        return f"{c_name}(self as {c_type_self})"

    elif self.is_copy_assignment_operator():
        c_type_self = self.parent().type().pointer_to().target_c()
        c_type_other = self.parent().type().with_const().pointer_to().target_c()

        return f"{c_name}(self as {c_type_self}, other as {c_type_other});"

    elif self.is_move_constructor():
        pass # XXX

    elif self.is_move_assignment_operator():
        pass # XXX

    else:
        c_parameters = ', '.join(p.name_interm() for p in self.parameters())

        call = f"{c_name}({c_parameters})"

    call = f"{self.return_type().output(outp=call)}"

    if not self.return_type().is_void():
        call = f"let {Id.RET} = {call};"

    if not self.is_noexcept():
        call = self.try_catch(call)

    return call


def _function_perform_return(self):
    if not self.return_type().is_void():
        return f"{Id.RET}" if self.is_noexcept() else f"Ok({Id.RET})"


def _function_try_catch(self, what):
    return code(
        """
        bind_error_reset();

        {what}

        if !bind_error_what().is_null() {{
            {handle_exception}
        }}
        """,
        what=what,
        handle_exception=self.handle_exception("bind_error_what()"))


def _function_handle_exception(self, what):
    return f"return Err(BindError::new({what}));"


def _function_forward(self):
    if self.is_copy_constructor() or \
       self.is_copy_assignment_operator() or \
       self.is_move_constructor() or \
       self.is_move_assignment_operator():
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


def _record_union(self):
    class union:
        @staticmethod
        def name_target(*args, **kwargs):
            return f"{self.name_target(*args, **kwargs)}Union"

    return union


def _record_trait(self):
    class trait:
        @staticmethod
        def name_target(*args, **kwargs):
            return f"{self.name_target(*args, **kwargs)}Trait"

    return trait


class RustPatcher(Patcher):
    def patch(self):
        self._patch(EnumConstant, 'name_target', _name(default_case=Id.SNAKE_CASE_CAP_ALL))
        self._patch(Function, 'declare_parameters', _function_declare_parameters)
        self._patch(Function, 'forward_call', _function_forward_call)
        self._patch(Function, 'try_catch', _function_try_catch)
        self._patch(Function, 'handle_exception', _function_handle_exception)
        self._patch(Function, 'perform_return', _function_perform_return)
        self._patch(Function, 'forward', _function_forward)
        self._patch(Id, 'reserved', _id_reserved)
        self._patch(Record, 'union', _record_union)
        self._patch(Record, 'trait', _record_trait)
        self._patch(Type, 'target_c', _type_target_c)
        self._patch(Type, 'target', _type_target)
        self._patch(Type, 'target', _type_target)
