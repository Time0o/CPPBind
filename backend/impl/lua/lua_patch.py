from cppbind import Function
from patch import Patcher


def _function_before_call(self):
    for p in self.parameters():
        if p.default_argument() is not None:
            return "function_call:"


class LuaPatcher(Patcher):
    def patch(self):
        self._function_before_call_orig = Function.before_call

        Function.before_call = _function_before_call

    def unpatch(self):
        Function.before_call = self._function_before_call_orig
