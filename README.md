# LPython

LPython is a Python compiler. It is in heavy development, currently in
pre-alpha stage. Some of the goals of LPython:

* The best possible performance for numerical, array-oriented code
* Run on all platforms
* Compile a subset of Python yet be fully compatible with Python
* Explore designs so that LPython eventually can compile all Python code
* Fast compilation
* Excellent user-friendly diagnostic messages: error, warnings, hints, notes,
  etc.
* Ahead-of-Time compilation to binaries, plus interactive usage (Jupyter
  notebook)
* Transforming Python code to C++, Fortran and other languages

And more.

# Installation

LPython works on Windows, macOS and Linux.

## Install Conda

Please follow the instructions here to install Conda on your platform:

https://github.com/conda-forge/miniforge/#download

### Linux

```bash
sudo apt install binutils-dev
```

### Windows
Install Visual Studio (MSVC), for example the version 2022, you can download the
Community version for free from: https://visualstudio.microsoft.com/downloads/.

Launch the Miniforge prompt from the Desktop.

In the shell, initialize the MSVC compiler using:

```bash
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd" -arch=x64
```

You can optionally test MSVC via:

```bash
cl /?
link /?
```

Both commands must print several pages of help text.

## Build LPython

Clone LPython

```bash
git clone https://github.com/lcompilers/lpython.git
cd lpython
```

### Linux and MacOS

- Create a Conda environment using the pre-existing file:

```bash
conda env create -f environment_unix.yml
conda activate lp
```

- Generate prerequisite files; build in Debug Mode:

```bash
./build0.sh
./build1.sh
```

### Windows

- Create a Conda environment using the pre-existing file:

```bash
conda env create -f environment_win.yml
conda activate lp
```

- Generate prerequisite files; build in Release Mode:

```bash
call build0.bat
call build1.bat
```

- Tests and examples

```bash
ctest
inst\bin\lpython examples\expr2.py
inst\bin\lpython examples\expr2.py -o a.out
a.out
```

## Tests (Linux or MacOs):

Run tests:

```bash
ctest
./run_tests.py
```

Run integration tests:

```bash
cd integration_tests
./run_tests.sh
```

### Speed up Integration Test on Macs

Integration tests run slowly because Apple checks the hash of each
executable online before running. You can turn off that feature
in the Privacy tab of the Security and Privacy item of System
Preferences, Developer Tools, Terminal.app, "allow the apps below
to run software locally that does not meet the system's security
policy."

## Examples (Linux or MacOs)

You can run the following examples by hand in a terminal:

```bash
./src/bin/lpython examples/expr2.py
./src/bin/lpython examples/expr2.py -o expr
./expr
./src/bin/lpython --show-ast examples/expr2.py
./src/bin/lpython --show-asr examples/expr2.py
./src/bin/lpython --show-cpp examples/expr2.py
./src/bin/lpython --show-llvm examples/expr2.py
./src/bin/lpython --show-c examples/expr2.py
```

## Contributing

We welcome contributions from anyone, even if you are new to compilers or to
open source. It might sound daunting to contribute to a compiler at first, but
please do, it is not complicated. We will help you with technical issues and
help improve your contribution so that it can be merged.

To contribute, submit a Pull Request (PR) against our repository at:

https://github.com/lcompilers/lpython

and don't forget to clean your history, see [example](./doc/src/rebasing.md).

Please report any bugs you may find at our issue tracker:
https://github.com/lcompilers/lpython/issues. Or, even better, fork the
repository on GitHub and create a PR. We welcome all changes, big or small, and
we will help you make a PR if you are new to git.

If you have any questions or need help, please ask us at Zulip ([![project
chat](https://img.shields.io/badge/zulip-join_chat-brightgreen.svg)](https://lfortran.zulipchat.com/))
or our [mailinglist](https://groups.io/g/lfortran).

See the [CONTRIBUTING](CONTRIBUTING.md) document for more information.
