from patcher import Patcher, _name
from pycppbind import Enum, Function, Identifier as Id, Type, Variable
from text import code


def _function_before_call(self):
    if self.is_constructor():
        t = self.parent().type()

        return f"char {Id.BUF}[{t.size()}];"


def _function_declare_return_value(self):
    return_type = self.return_type()

    if return_type.is_void():
        return

    if return_type.is_record_indirection():
        return_type = return_type.pointee().without_const()

    return f"{return_type.target()} {Id.RET};"


def _function_perform_return(self):
    if self.return_type().is_void():
        return

    return f"return {Id.RET};"


def _function_construct(self, parameters):
    t = self.parent().type()

    return f"new ({Id.BUF}) {t}({parameters});"


def _function_destruct(self):
    this = self.parameters()[0].name_interm()

    t = self.parent().type()

    return f"{this}->~{t.format(quals=Id.REMOVE_QUALS)}();"


def _function_handle_exception(self, what):
    if self.return_type().is_void():
        return f"bind_error = {what};"

    return code(
        f"""
        bind_error = {what};
        return {{}};
        """)


class CPatcher(Patcher):
   def patch(self):
        self._patch(Enum, 'name_target', _name(default_case=Id.SNAKE_CASE))
        self._patch(Variable, 'name_target', _name(default_case=Id.SNAKE_CASE_CAP_ALL,
                                                   default_quals=Id.REPLACE_QUALS))
        self._patch(Function, 'before_call', _function_before_call)
        self._patch(Function, 'declare_return_value', _function_declare_return_value)
        self._patch(Function, 'perform_return', _function_perform_return)
        self._patch(Function, 'construct', _function_construct)
        self._patch(Function, 'destruct', _function_destruct)
        self._patch(Function, 'handle_exception', _function_handle_exception)
