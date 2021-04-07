# Installation

All the instructions below work on Linux, macOS and Windows.

## Binaries

The recommended way to install LFortran is using Conda.
Install Conda for example by installing the
[Miniconda](https://conda.io/en/latest/miniconda.html) installation by following instructions there for your platform.
Then create a new environment (you can choose any name, here we chose `lf`) and
activate it:
```bash
conda create -n lf
conda activate lf
```
Then install LFortran by:
```bash
conda install lfortran -c conda-forge
```
Now the `lf` environment has the `lfortran` compiler available, you can start the
interactive prompt by executing `lfortran`, or see the command line options using
`lfortran -h`.

The Jupyter kernel is automatically installed by the above command, so after installing Jupyter itself:
```bash
conda install jupyter -c conda-forge
```
You can create a Fortran based Jupyter notebook by executing:
```bash
jupyter notebook
```
and selecting `New->Fortran`.


## Build From a Source Tarball

This method is the recommended method if you just want to install LFortran, either yourself or in a package manager (Spack, Conda, Debian, etc.). The source tarball has all the generated files included and has minimal dependencies.

First we have to install dependencies, for example using Conda:
```bash
conda create -n lf python cmake llvmdev
conda activate lf
```
Then download a tarball from 
[https://lfortran.org/download/](https://lfortran.org/download/), 
e.g.:
```bash
wget https://lfortran.github.io/tarballs/dev/lfortran-0.9.0.tar.gz
tar xzf lfortran-0.9.0.tar.gz
cd lfortran-0.9.0
```
And build:
```
cmake -DWITH_LLVM=yes -DCMAKE_INSTALL_PREFIX=`pwd`/inst .
make -j8
make install
```
This will install the `lfortran` into the `inst/bin`.

## Build From Git

We assume you have C++ compilers installed, as well as `git` and `wget`.
In Ubuntu, you can also install `binutils-dev` for stacktraces.

If you do not have Conda installed, you can do so on Linux (and similarly on
other platforms):
```bash
wget --no-check-certificate https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh -O miniconda.sh
bash miniconda.sh -b -p $HOME/conda_root
export PATH="$HOME/conda_root/bin:$PATH"
```
Then prepare the environment:
```bash
conda create -n lf -c conda-forge llvmdev=11.0.1 bison=3.4 re2c python cmake make toml
conda activate lf
```
Clone the LFortran git repository:
```
git clone https://gitlab.com/lfortran/lfortran.git
cd lfortran
```
Generate files that are needed for the build (this step depends on `re2c`, `bison` and `python`):
```bash
./build0.sh
```
Now the process is the same as installing from the source tarball. For example to build in Debug mode:
```
cmake -DCMAKE_BUILD_TYPE=Debug -DWITH_LLVM=yes -DCMAKE_INSTALL_PREFIX=`pwd`/inst .
make -j8
```

Run tests:
```bash
ctest
./run_tests.py
```
Run an interactive prompt:
```bash
./src/bin/lfortran
```

## Enabling the Jupyter Kernel

To install the Jupyter kernel, install the following Conda packages also:
```
conda install xeus xtl nlohmann_json cppzmq
```
and enable the kernel by `-DWITH_XEUS=yes` and install into `$CONDA_PREFIX`. For
example:
```
cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DWITH_LLVM=yes \
    -DWITH_XEUS=yes \
    -DCMAKE_PREFIX_PATH="$CONDA_PREFIX" \
    -DCMAKE_INSTALL_PREFIX="$CONDA_PREFIX" \
    .
cmake --build . -j4 --target install
```
To use it, install Jupyter (`conda install jupyter`) and test that the LFortran
kernel was found:
```
jupyter kernelspec list --json
```
Then launch a Jupyter notebook as follows:
```
jupyter notebook
```
Click `New->Fortran`. To launch a terminal jupyter LFortran console:
```
jupyter console --kernel=fortran
```


## Build From Git with Nix

One of the ways to ensure exact environment and dependencies is with `nix`. This will ensure that system dependencies do not interfere with the development environment. If you want, you can report bugs in a `nix-shell` environment to make it easier for others to reproduce.

### With Root

We start by getting `nix`. The following multi-user installation will work on any machine with a Linux distribution, MacOS or Windows (via WSL):
```bash
sh <(curl -L https://nixos.org/nix/install) --daemon
```
### Without Root

If you would like to not provide `nix` with root access to your machine, on Linux distributions we can use [nix-portable](https://github.com/DavHau/nix-portable).
```bash
wget https://github.com/DavHau/nix-portable/releases/download/v003/nix-portable
```
Now just prepend all `nix-shell` commands with `NP_RUNTIME=bwrap ./nix-portable `. So:
```bash
# Do not
nix-shell --run "bash"
# Do
NP_RUNTIME=bwrap ./nix-portable nix-shell --run "bash"
```

### Development

Now we can enter the development environment:
```bash
nix-shell --run "bash" --cores 4 -j4 --pure ci/shell.nix
```
The `--pure` flag ensures no system dependencies are used in the environment.

The build steps are the same as with the `ci`:
```bash
./build0.sh
./build1.sh
```

To change the compilation environment from `gcc` (default) to `clang` we can use `--argstr`:
```bash
nix-shell --run "bash" --cores 4 -j4 --pure ci/shell.nix --argstr clangOnly "yes"
```

## Note About Dependencies

End users (and distributions) are encouraged to use the tarball
from [https://lfortran.org/download/](https://lfortran.org/download/),
which only depends on LLVM, CMake and a C++ compiler.

The tarball is generated automatically by our CI (continuous integration) and
contains some autogenerated files: the parser, the AST and ASR nodes, which is generated by an ASDL
translator (requires Python).

The instructions from git are to be used when developing LFortran itself.

## Note for users who do not use Conda

Following are the dependencies necessary for installing this
repository in development mode,

- [Bison - 3.5.1](https://ftp.gnu.org/gnu/bison/bison-3.5.1.tar.xz)
- [LLVM - 11.0.1](https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.1/llvm-11.0.1.src.tar.xz)
- [re2c - 2.0.3](https://re2c.org/install/install.html)
- [binutils - 2.31.90](ftp://sourceware.org/pub/binutils/snapshots/binutils-2.31.90.tar.xz) - Make sure that you should enable the required options related to this dependency to build the dynamic libraries (the ones ending with `.so`).

## Stacktraces

LFortran can print stacktraces when there is an unhandled exception, as well as
on any compiler error with the `--show-stacktrace` option. This is very helpful
for developing the compiler itself to see where in LFortran the problem is. The
stacktrace support is turned off by default, to enable it, install `binutils`
and compile LFortran with the `-DWITH_STACKTRACE=yes` cmake option.

In Ubuntu, `apt install binutils-dev`.

On macOS, you can install [Spack](https://spack.io/), then:
```
spack install binutils
spack find -p binutils
```
The last command will show a full path to the installed `binutils` package. Add
this path to your shell config file, e.g.:
```
export CMAKE_PREFIX_PATH_LFORTRAN=/Users/ondrej/repos/spack/opt/spack/darwin-catalina-broadwell/apple-clang-11.0.0/binutils-2.36.1-wy6osfm6bp2323g3jpv2sjuttthwx3gd
```
and compile LFortran with the
`-DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH_LFORTRAN;$CONDA_PREFIX"` cmake option.
The `$CONDA_PREFIX` is there if you install some other dependencies (such as
`llvm`) using Conda, otherwise you can remove it.
