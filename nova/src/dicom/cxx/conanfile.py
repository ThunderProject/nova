from conan import ConanFile
from conan.tools.cmake import cmake_layout


class DicomRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def configure(self):
        self.options["dcmtk"].with_charls = True
    def requirements(self):
        self.requires("gdcm/3.0.24")
        self.requires("opencv/4.11.0")
        self.requires("libassert/2.1.4")
        self.requires("nlohmann_json/3.12.0")
        self.requires("unordered_dense/4.5.0")
        self.requires("quill/9.0.2")

    def layout(self):
        cmake_layout(self)