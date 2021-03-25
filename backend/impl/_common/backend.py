import os

from abc import abstractmethod
from cppbind import Options
from file import File, Path
from itertools import chain
from util import Generic


class Backend(metaclass=Generic):
    def __init__(self, input_file, wrapper):
        self._input_file = Path(input_file)
        self._output_files = []

        self._wrapper = wrapper
        self._includes = wrapper.includes()
        self._constants = wrapper.constants()
        self._records = wrapper.records()
        self._functions = wrapper.functions()

        self._types = set()

        for r in self._records:
            self._types.add(r.type())

        for v in self._constants:
            self._types.add(v.type())

        for f in chain(self._functions, *(r.functions() for r in self._records)):
            for t in [f.return_type()] + [p.type() for p in f.parameters()]:
                self._types.add(t)

    def run(self):
        self.wrap_before()

        for v in self._constants:
            self.wrap_constant(v)

        for r in self._records:
            self.wrap_record(r)

        for f in self._functions:
            self.wrap_function(f)

        self.wrap_after()

        for output_file in self._output_files:
            output_file.write()

    def input_file(self):
        return self._input_file

    def input_includes(self, relative=None):
        if relative is None:
            relative = Options.output_relative_includes

        return [inc.str(relative=relative) for inc in self._includes]

    def output_file(self, output_path):
        output_dir = os.path.abspath(Options.output_directory)

        output_file = File(output_path.modified(dirname=output_dir))

        self._output_files.append(output_file)

        return output_file

    def wrapper(self):
        return self._wrapper

    def constants(self):
        return self._constants

    def records(self):
        return self._records

    def functions(self):
        return self._functions

    def types(self):
        return self._types

    @abstractmethod
    def wrap_before(self):
        pass

    @abstractmethod
    def wrap_after(self):
        pass

    @abstractmethod
    def wrap_constant(self, c):
        pass

    @abstractmethod
    def wrap_record(self, r):
        pass

    @abstractmethod
    def wrap_function(self, f):
        pass
