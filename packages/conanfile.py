from conan import ConanFile
from conan.tools.cmake import cmake_layout


class ExampleRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("dcmtk/3.6.9")
        self.requires("catch2/3.13.0")
        self.requires("libassert/2.2.1")
        self.requires("quill/11.0.2")
        self.requires("magic_enum/0.9.7")

    def layout(self):
        cmake_layout(self)
