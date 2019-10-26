# coding=utf8

from conans import ConanFile

class Pipeable(ConanFile):
    name = "pipeable"
    version = "0.1"
    description = \
        "Enables easy chaining of callable objects, automatically forwarding output as input: \n" \
        "auto result = vector{1, 2, 3} >>= for_each >>= int_to_cat >>= cat_to_hat;"
    author = "Fred Helmesjö (helmesjo@live.com)"
    url = "https://github.com/helmesjo/pipeable.git"
    license = "MIT"
    # No settings/options are necessary, this is header only
    exports_sources = "include/*", "LICENSE", "README.*"
    no_copy_source = True

    def package(self):
        self.copy("*.hpp")
    def package_id(self):
        self.info.header_only()