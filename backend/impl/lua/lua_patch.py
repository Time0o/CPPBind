from cppbind import Function


def _function_before_call(self):
    for p in self.parameters():
        if p.default_argument() is not None:
            return "function_call:"


Function.before_call = _function_before_call
