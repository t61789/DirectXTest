import os

from conan import ConanFile
from conan.tools.cmake import cmake_layout
from conan.tools.files import copy


class ImGuiExample(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"
    
    def requirements(self):
        self.requires("nlohmann_json/3.12.0")
        self.requires("directx-headers/1.614.0")
        self.requires("assimp/5.4.3")
        self.requires("tracy/0.12.1")
        self.requires("boost/1.88.0")
        self.requires("stb/cci.20230920")
        self.requires("imgui/1.92.5")
        
    def layout(self):
        cmake_layout(self)
        
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()