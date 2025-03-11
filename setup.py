# setup.py
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

class CustomBuildExt(build_ext):
    def build_extension(self, ext):
        # Remove -bundle from linker args and enforce -dynamiclib
        self.compiler.linker_so = [
            arg for arg in self.compiler.linker_so if arg != "-bundle"
        ]
        # Ensure -dynamiclib is included
        if "-dynamiclib" not in ext.extra_link_args:
            ext.extra_link_args.append("-dynamiclib")
        super().build_extension(ext)

tfm_extension = Extension(
    "msurf.libtfm_pywrapper",
    sources=["src/c/tfm_opencl.c"],
    include_dirs=["include"],
    extra_compile_args=["-std=c99", "-Wall", "-Wextra", "-Werror", "-pedantic", "-g", "-fPIC"],
    extra_link_args=["-dynamiclib", "-undefined", "dynamic_lookup"]
)

setup(
    ext_modules=[tfm_extension],
    cmdclass={"build_ext": CustomBuildExt}
)
