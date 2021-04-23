from cppbind import Constant, Function, Identifier as Id, Type
from patch import Patcher
from text import code


def _constant_name_target(self):
    return self.name().format(case=Id.SNAKE_CASE_CAP_ALL,
                              quals=Id.REPLACE_QUALS)


def _constant_declare(self):
    return f"extern {self.type().target()} {self.name_target()};"


def _constant_assign(self):
    return self.type().output(
        outp=self.name(),
        interm=f"{self.type().target()} {self.name_target()}")


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


def _function_destruct(self, this):
    t = self.parent().type()

    return f"{this}->~{t.format(quals=Id.REMOVE_QUALS)}();"


def _function_handle_exception(self, what):
    if self.return_type().is_void():
        return "errno = EBIND;"

    return code(
        """
        errno = EBIND;
        return {};
        """)


class CPatcher(Patcher):
    def patch(self):
        self._constant_name_target_orig = Constant.name_target
        self._constant_declare_orig = Constant.declare
        self._constant_assign_orig = Constant.assign
        self._function_before_call_orig = Function.before_call
        self._function_declare_return_value_orig = Function.declare_return_value
        self._function_perform_return_orig = Function.perform_return
        self._function_construct_orig = Function.construct
        self._function_destruct_orig = Function.destruct
        self._function_handle_exception_orig = Function.handle_exception

        Constant.name_target = _constant_name_target
        Constant.declare = _constant_declare
        Constant.assign = _constant_assign
        Function.before_call = _function_before_call
        Function.declare_return_value = _function_declare_return_value
        Function.perform_return = _function_perform_return
        Function.construct = _function_construct
        Function.destruct = _function_destruct
        Function.handle_exception = _function_handle_exception

    def unpatch(self):
        Constant.name_target = self._constant_name_target_orig
        Constant.declare = self._constant_declare_orig
        Constant.assign = self._constant_assign_orig
        Function.before_call = self._function_before_call_orig
        Function.declare_return_value = self._function_declare_return_value_orig
        Function.perform_return = self._function_perform_return_orig
        Function.construct = self._function_construct_orig
        Function.destruct = self._function_destruct_orig
        Function.handle_exception = self._function_handle_exception_orig
