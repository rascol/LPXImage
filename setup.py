import os
import re
import sys
import platform
import subprocess
import setuptools

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

# Define version
__version__ = '0.1.0'

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)

class CMakeBuild(build_ext):
    def run(self):
        try:
            subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build the extension")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(
            os.path.dirname(self.get_ext_fullpath(ext.name)))
        
        # Required for auto-detection & inclusion of auxiliary "native" libs
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep

        cmake_args = [
            '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
            '-DPYTHON_EXECUTABLE=' + sys.executable,
            '-DBUILD_PYTHON_BINDINGS=ON'
        ]

        # Platform-specific options
        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(
                cfg.upper(), extdir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j4']

        # macOS specific options
        if platform.system() == "Darwin":
            cmake_args += [
                '-DCMAKE_INSTALL_RPATH=/usr/local/lib',
                '-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON',
                '-DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON',
                '-DCMAKE_MACOSX_RPATH=ON'
            ]

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(
            env.get('CXXFLAGS', ''), self.distribution.get_version())
        
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
            
        subprocess.check_call(
            ['cmake', ext.sourcedir] + cmake_args, 
            cwd=self.build_temp, 
            env=env
        )
        subprocess.check_call(
            ['cmake', '--build', '.'] + build_args, 
            cwd=self.build_temp
        )

# Read README for the long description
with open('README.md', 'r') as f:
    long_description = f.read()

setup(
    name='lpximage',
    version=__version__,
    author='LPXImage Team',
    author_email='example@example.com',
    description='Python bindings for the LPXImage library',
    long_description=long_description,
    long_description_content_type='text/markdown',
    url='https://github.com/rascol/LPXImage',
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
    ],
    python_requires='>=3.6',
    ext_modules=[CMakeExtension('lpximage')],
    cmdclass={'build_ext': CMakeBuild},
    zip_safe=False,
    install_requires=[
        'numpy>=1.19.0',
        'opencv-python>=4.5.0',
        'pybind11>=2.6.0',
    ],
)
