from abc import abstractmethod
from file import File, Path
from util import Generic


class Backend(metaclass=Generic):
    def __init__(self, input_file, wrapper, options):
        self._input_file = Path(input_file)
        self._output_files = []

        self._wrapper = wrapper
        self._constants = wrapper.constants()
        self._records = wrapper.records()
        self._functions = wrapper.functions()

        self._options = options

    def run(self):
        self.wrap_before()

        for v in self._constants:
            self.wrap_constant(v)

        for r, _ in self._records:
            self.wrap_record(r)

        for f in self._functions:
            self.wrap_function(f)

        self.wrap_after()

        for output_file in self._output_files:
            output_file.write()

    def input_file(self):
        return self._input_file

    def output_file(self, output_path):
        output_dir = self.option('output-directory')

        if output_dir:
            output_path = output_path.modified(dirname=output_dir)

        output_file = File(output_path)

        self._output_files.append(output_file)

        return output_file

    def wrapper(self):
        return self._wrapper

    def constants(self):
        return self._constants

    def records(self, with_bases=False):
        if with_bases:
            return self._records

        return [r for r, _ in self._records]

    def functions(self):
        return self._functions

    def option(self, name):
        try:
            getter = getattr(self._options, name)
        except AttributeError:
            raise ValueError(f"no such option: '{name}'")

        return getter()

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
