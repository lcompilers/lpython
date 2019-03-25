# LFortran Documentation

LFortran is a modern open-source (BSD licensed) interactive Fortran compiler
built on top of LLVM.
It can execute user's code interactively to allow exploratory work (much like
Python, MATLAB or Julia) as well as compile to binaries with the goal to run
user's code on modern architectures such as multi-core CPUs and GPUs.

Main repository:

[https://gitlab.com/lfortran/lfortran](https://gitlab.com/lfortran/lfortran)

Try online using Binder:
[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gl/lfortran%2Fweb%2Flfortran-binder/master?filepath=Demo.ipynb)

## Feature Highlights

LFortran is in development, the features below either work, or are being
implemented (in which case there is a link to the corresponding issue so that
you can track the progress by following it).

* **Interactive, Jupyter support**  
    LFortran can be used from Jupyter as a Fortran kernel, allowing a
    Python/Julia style rapid prototyping and exploratory workflow
    ([example notebook](https://nbviewer.jupyter.org/gist/certik/f1d28a486510810d824869ab0c491b1c)).
    Or it can be used from the command-line in an interactive prompt (REPL).

* **Native interoperation with other languages (and other Fortran compilers)**  
    It can automatically call code written in other languages (such as C or
    Python) just by using the `use` statement, see
    [#44](https://gitlab.com/lfortran/lfortran/issues/44). It understands
    other Fortran compilers module files (one can just "use" them) and their
    ABI to link correctly (GFortran is supported, other compilers are planned,
    see [#56](https://gitlab.com/lfortran/lfortran/issues/56)), which allows to
    use LFortran with production codes today.

* **Clean, modular design, usable as a library**  
    LFortran is structured around two independent modules, AST and ASR, both of
    which are standalone (completely independent of the rest of LFortran) and
    users are encouraged to use them independently for other applications and
    build tools on top. See the [Design](design.md) document for more details.

* **Create executables**  
    It can create executables just like other Fortran compilers.

* **Modern hardware support**  
    Thanks to LLVM, the goal of LFortran is to run on modern hardware and take
    advantage of native Fortran language constructs (such as `do concurrent`)
    to run on multi-core CPUs and GPUs, see
    [#57](https://gitlab.com/lfortran/lfortran/issues/57).

* **Full Fortran 2018 support**  
    Currently only a subset of Fortran is implemented, but the goal is to have
    a full implementation of the latest Fortran 2018 standard, see
    [#58](https://gitlab.com/lfortran/lfortran/issues/58).


Please vote on issues in our
[issue tracker](https://gitlab.com/lfortran/lfortran/issues) that you want us
to prioritize (feel free to create new ones if we are missing anything).
