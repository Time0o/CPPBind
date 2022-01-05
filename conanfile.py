from conans import ConanFile, CMake, tools

class BuenzliCoindConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    requires = [
        ('boost/1.78.0'),
        ('pybind11/2.8.1'),
    ]

    generators = "cmake_find_package"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
