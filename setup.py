import os
import setuptools
import sys
import skbuild

with open("README.md", "r") as fh:
    long_description = fh.read()

with open("version", "r") as fh:
    version = fh.read().strip()

# scikit-build sometimes passes the incorrect MACOSX_DEPLOYMENT_TARGET value to
# CMake. This ensures that the correct value is always passed in:
cmake_args = []
mdt = os.getenv("MACOSX_DEPLOYMENT_TARGET")
if mdt:
    cmake_args.append("-DCMAKE_OSX_DEPLOYMENT_TARGET={}".format(mdt))

if sys.platform == "win32":
    cmake_args.append("-DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE")

cmake_args.append("-DWITH_PYTHON=TRUE")

skbuild.setup(
    name="lfortran",
    version=version,
    author="Ondřej Čertík",
    author_email="ondrej@certik.us",
    description="Fortran compiler",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://gitlab.com/lfortran/lfortran",
    packages=setuptools.find_packages(),
    include_package_data=True,
    data_files=[
        ('share/lfortran/nb', ['share/lfortran/nb/Demo.ipynb']),
        ],
    scripts=['lfort'],
    cmake_args = cmake_args,
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    cmake_languages=('C'),
)
