import setuptools
import skbuild

with open("README.md", "r") as fh:
    long_description = fh.read()

with open("version", "r") as fh:
    version = fh.read().strip()

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
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    cmake_languages=('C'),
    install_requires=[
        "pytest",
        "llvmlite",
        "prompt_toolkit",
        "antlr4-python3-runtime",
        "pygments",
    ],
)
