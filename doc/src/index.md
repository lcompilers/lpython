# LFortran Documentation

LFortran is a modern open-source (BSD licensed) interactive Fortran compiler
built on top of LLVM.
It can execute user's code interactively to allow exploratory work (much like
Python, MATLAB or Julia) as well as compile to binaries with the goal to run
user's code on modern architectures such as multi-core CPUs and GPUs.

## Feature Highlights

* **Interactive, Jupyter support**  
    LFortran can be used from Jupyter as a Fortran kernel, allowing a
    Python/Julia style rapid prototyping and exploratory workflow. Or it can be
    used from the command-line in an interactive prompt (REPL).

* **Native interoperation with other languages (and other Fortran compilers)**  
    It can automatically call into C and Python just by using the `use`
    statement (e.g., by parsing the C header files using Clang). It understands
    other Fortran compilers module files (one can just "use" them) and their
    ABI to link correctly (for now only GFortran is supported, other compilers
    are planned).

* **Clean, modular design, usable as a library**  
    LFortran is structured around two independent modules, AST and ASR, both of
    which are standalone (completely independent of the rest of LFortran) and
    users are encouraged to use them independently for other applications and
    build tools on top. See the [Design](design.md) document for more details.

* **Create executables**  
    It can create executables just like other Fortran compilers.

* **Modern hardware support**  
    Thanks to LLVM, the goal of LFortran is to run on modern hardware and take
    advantage of native Fortran constructs (such as `do concurrent`) to run on
    multi-core CPUs and GPUs.
