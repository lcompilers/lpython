# LPython

LPython is a Python compiler. It is in heavy development, currently in alpha
stage. LPython works on Windows, macOS and Linux. Some of the goals of LPython
include:

- The best possible performance for numerical, array-oriented code
- Run on all platforms
- Compile a subset of Python yet be fully compatible with Python
- Explore designs so that LPython eventually can compile all Python code
- Fast compilation
- Excellent user-friendly diagnostic messages: error, warnings, hints, notes,
  etc.
- Ahead-of-Time compilation to binaries, plus interactive usage (Jupyter notebook)
- Transforming Python code to C++, Fortran and other languages

And more.

# Sponsors

LPython has been sponsored by [GSI Technology](https://www.gsitechnology.com/).
Our summer students were sponsored by Google Summer of Code via Python Software
Foundation. The intermediate representation and backends are shared with
LFortran, see that project for a list of sponsors.

# Installation

## Step 0: Prerequisites

Here is the list of requirements needed to build LPython:

- Python (3.10+)
- Conda

For Windows, these are additionally required:

- Miniforge Prompt
- Visual Studio (with "Desktop Development with C++" workload)

Please follow the steps for your desired platform.

## Step 1: Install Conda

This step involves installing Conda using a conda-forge distribution called Miniforge.

Please follow the instructions here to install Conda on your platform:

Miniforge download link (for Linux, MacOS and Windows): https://github.com/conda-forge/miniforge/#download

## Step 2: Setting up

This step involves setting up the required configuration to run the programs in LPython.

### Linux

Run the below command to install `binutils-dev` package on Linux.

```bash
sudo apt install binutils-dev
```

### Windows

Please follow the below steps for Windows:

- Install Visual Studio, for example the version 2022.

  - You can download the
    Community version for free from: https://visualstudio.microsoft.com/downloads/.
  - After installing Visual Studio and running the Visual Studio Installer, you must install the "Desktop Development with C++" workload which will install Visual C++ Compiler (MSVC).

- Launch the Miniforge prompt from the Desktop.

  - It is recommended to use MiniForge instead of Powershell as the main terminal to build and write code for LPython.

- In the MiniForge Prompt, initialize the MSVC compiler using the below command:

  ```bash
  call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd" -arch=x64
  ```

- You can optionally test MSVC via:

  ```bash
  cl /?
  link /?
  ```

  Both commands must print several pages of help text.

## Step 3: Build LPython

- Clone LPython using the following commands

  ```bash
  git clone https://github.com/lcompilers/lpython.git
  cd lpython
  ```

  You may also use GitHub Desktop to do the same.

### Linux and MacOS

- Create a Conda environment using the pre-existing file:

  ```bash
  conda env create -f environment_unix.yml
  conda activate lp
  ```

- Generate prerequisite files; build in Debug Mode:

  ```bash
  # if you are developing on top of a forked repository; please run following command first
  # ./generate_default_tag.sh


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

- Whenever you are updating a test case file, you also need to update all the reference results associated with that test case:

  ```
  python run_tests.py -u --skip-run-with-dbg
  ```

- To see all the options associated with LPython test suite, use:

  ```
  python run_tests.py --help
  ```

## Tests:

### Linux or MacOS

- Run tests:

  ```bash
  ctest
  ./run_tests.py
  ```

- Run integration tests:

  ```bash
  cd integration_tests
  ./run_tests.py
  ```

### Windows

- Run integration tests

  ```bash
  python run_tests.py --skip-run-with-dbg
  ```

- Update reference tests

  ```bash
  python run_tests.py -u --skip-run-with-dbg
  ```

## Speed up Integration Tests on MacOS

Integration tests run slowly because Apple checks the hash of each
executable online before running.

You can turn off that feature in the Privacy tab of the Security and Privacy item of System Preferences > Developer Tools > Terminal.app > "allow the apps below
to run software locally that does not meet the system's security
policy."

## Examples (Linux or MacOS)

You can run the following examples manually in a terminal:

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

# Star History

[![Star History Chart](https://api.star-history.com/svg?repos=lcompilers/lpython&type=Date)](https://star-history.com/#lcompilers/lpython&Date)
