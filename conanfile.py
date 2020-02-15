# coding=utf8

from conans import ConanFile, CMake
import tempfile

class Pipeable(ConanFile):
    name = "pipeable"
    version = "0.1"
    description = \
        "Enables easy chaining of callable objects, automatically forwarding output as input: \n" \
        "auto result = vector{1, 2, 3} >>= for_each >>= int_to_cat >>= cat_to_hat;"
    author = "Fred Helmesjö <helmesjo@gmail.com>"
    url = "https://github.com/helmesjo/pipeable.git"
    license = "MIT"
    exports_sources = "*", "!build", "LICENSE", "README.*"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "build_tests": [True, False]
    }
    default_options = {
        "build_tests": True
    }

    def build(self):
        cmake = CMake(self, set_cmake_flags=True)
        cmake.definitions["PIPEABLE_BUILD_TESTS"] = self.options.build_tests
        cmake.configure(build_dir="build")
        cmake.build(build_dir="build")

        if self.options.build_tests:
            cmake.test(build_dir="build")

    def package(self):
        self.copy("include/*.hpp")
    def package_id(self):
        self.info.header_only()
