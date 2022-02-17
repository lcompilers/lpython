# LPython


LPython is a Python compiler. It is in heavy development, currently in
pre-alpha stage. Some of the goals of LPython:

* The best possible performance for numerical array oriented code
* Run on all platforms
* Compile a subset of Python and be Python compatible
* Explore how to design it so that it can be eventually used with any Python
  code
* Fast compilation
* Excellent user friendly diagnostic messages: error, warnings, hints, notes,
  etc.
* Ahead of time compilation to binaries and interactive usage (Jupyter
  notebook)
* Able to transform the Python code to C++, Fortran and other languages

And more.

# Installation

LPython works on Windows, macOS and Linux.

## Install Conda

If you do not have Conda already installed, please follow the instructions
here to install Conda on your platform:

https://github.com/conda-forge/miniforge/#download

## Compile LPython

Create a Conda environment:

    conda create -n lp llvmdev=11.0.1 bison=3.4 re2c python cmake make toml
    conda activate lp

Clone LPython

    git clone https://github.com/lcompilers/lpython.git
    cd lpython

Create autogenerated files (choose the command for your platform):

    ./build0.sh      # macOS/Linux
    call build0.bat  # Windows

Compile LPython:

    cmake -DCMAKE_BUILD_TYPE=Debug -DWITH_LLVM=yes -DWITH_STACKTRACE=yes -DWITH_LFORTRAN_BINARY_MODFILES=no .
    cmake --build . -j16

## Tests:

Run tests:

    ctest
    ./run_tests.py

## Examples

You can run the following examples by hand in a terminal:

    ./src/bin/lpython examples/expr2.py
    ./a.out
    ./src/bin/lpython --show-ast examples/expr2.py
    ./src/bin/lpython --show-asr examples/expr2.py
    ./src/bin/lpython --show-cpp examples/expr2.py
    ./src/bin/lpython --show-llvm examples/expr2.py

## Contributing

We welcome contributions from anyone, even if you are new to open source. It
might sound daunting to contribute to a compiler at first, but please do, it is
not complicated. We will help you with any technical issues and help improve
your contribution so that it can be merged.

To contribute, submit a Pull Request (PR) against our repository at:

https://github.com/lcompilers/lpython

Please report any bugs you may find at our issue tracker: https://github.com/lcompilers/lpython/issues.
Or, even better, fork the repository on GitHub and create a PR. We welcome all changes, big or small, and we will help you make a PR if you are new to git.

If you have any questions or need help, please ask us at Zulip ([![project chat](https://img.shields.io/badge/zulip-join_chat-brightgreen.svg)](https://lfortran.zulipchat.com/)) or our
[mailinglist](https://groups.io/g/lfortran).

See the [CONTRIBUTING](CONTRIBUTING.md) document for more information.
