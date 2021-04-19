from cppbind import Constant, Function, Identifier as Id, Type
from patch import Patcher


def _constant_name_target(self):
    if self.is_macro():
        return 'M_' + self.name().format(case=Id.SNAKE_CASE_CAP_ALL)
    else:
        return self.name().format(case=Id.SNAKE_CASE_CAP_ALL,
                                  quals=Id.REPLACE_QUALS)


def _function_before_call(self):
    if self.is_constructor():
        t = self.parent().type()

        return f"char {Id.TMP}[{t.size()}];"


def _function_construct(self, parameters):
    t = self.parent().type()

    return f"new ({Id.TMP}) {t}({parameters});"


def _function_destruct(self, this):
    t = self.parent().type()

    return f"{this}->~{t.format(quals=Id.REMOVE_QUALS)}();"


class CPatcher(Patcher):
    def patch(self):
        self._constant_name_target_orig = Constant.name_target
        self._function_before_call_orig = Function.before_call
        self._function_construct_orig = Function.construct
        self._function_destruct_orig = Function.destruct

        Constant.name_target = _constant_name_target
        Function.before_call = _function_before_call
        Function.construct = _function_construct
        Function.destruct = _function_destruct

    def unpatch(self):
        Constant.name_target = self._constant_name_target_orig
        Function.before_call = self._function_before_call_orig
        Function.construct = self._function_construct_orig
        Function.destruct = self._function_destruct_orig
