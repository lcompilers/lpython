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

LFortran is in development, there are features that work today, and there are
features that are being implemented.

### Works today

* **Interactive, Jupyter support**  
    LFortran can be used from Jupyter as a Fortran kernel, allowing a
    Python/Julia style rapid prototyping and exploratory workflow
    (see the [static](https://nbviewer.jupyter.org/gist/certik/f1d28a486510810d824869ab0c491b1c)
    or
    [interactive](https://mybinder.org/v2/gl/lfortran%2Fweb%2Flfortran-binder/master?filepath=Demo.ipynb)
    example notebook).
    Or it can be used from the command-line in an interactive prompt (REPL).

* **Clean, modular design, usable as a library**  
    LFortran is structured around two independent modules, AST and ASR, both of
    which are standalone (completely independent of the rest of LFortran) and
    users are encouraged to use them independently for other applications and
    build tools on top. See the [Design](design.md)
    and [Developer Tutorial](developer_tutorial.md) documents for more details.

* **Interoperation with GFortran**  
    LFortran can parse GFortran module files into an ASR and generate a Fortran
    wrapper that can be compiled with any Fortran compiler and linked with the
    original GFortran compiled module.

* **Create executables**  
    It can create executables just like other Fortran compilers.

* **Runs on Linux and Windows**  
    Regularly tested on Linux and Windows. Mac support is planned
    ([#67](https://gitlab.com/lfortran/lfortran/issues/67)).


### Planned

These features are under development, there is a link to the corresponding
issue so that you can track the progress by following it.

* **Native interoperation with other languages (and other Fortran compilers)**  
    It can automatically call code written in other languages (such as C or
    Python) just by using the `use` statement, see
    [#44](https://gitlab.com/lfortran/lfortran/issues/44). It understands
    other Fortran compilers module files (one can just "use" them) and their
    ABI to link correctly (GFortran is supported, other compilers are planned,
    see [#56](https://gitlab.com/lfortran/lfortran/issues/56)), which allows to
    use LFortran with production codes today.

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

## Roadmap

Here is our roadmap how to get all the planned features above implemented:

1. Port code generation to use ASR and pass all the current tests
([#74](https://gitlab.com/lfortran/lfortran/issues/74)).
Remove the old code generation and old semantics, that used to annotate the AST tree, which was messy.

2. Get GFortran module files working with `use` module, both assumed-size and
assumed-shape arrays ([#52](https://gitlab.com/lfortran/lfortran/issues/52)).
This will allow right away to use production codes with LFortran.

3. Do these at the same time:

    a) Implement more Fortran features, until eventually full Fortran 2018 is
    supported ([#58](https://gitlab.com/lfortran/lfortran/issues/58)).

    b) Gradually move to C++ for robustness and speed
    ([#70](https://gitlab.com/lfortran/lfortran/issues/70)).
    The Python API will not change (or only minimally), so the user experience
    from Python will not change.

    c) Implement all the other cool features: `use` module for C and Python
    and automatic wrappers
    ([#44](https://gitlab.com/lfortran/lfortran/issues/44)),
    modern hardware support
    ([#57](https://gitlab.com/lfortran/lfortran/issues/57)),
    generating an older standard of Fortran
    ([#72](https://gitlab.com/lfortran/lfortran/issues/72)),
    Fortran doctest feature
    ([#73](https://gitlab.com/lfortran/lfortran/issues/73)),
    SymPy integration
    ([#71](https://gitlab.com/lfortran/lfortran/issues/71)),
    language service for IDEs
    ([#12](https://gitlab.com/lfortran/lfortran/issues/12)),
    and other ideas ([#29](https://gitlab.com/lfortran/lfortran/issues/29)).

The step 1. is an internal refactoring that will not take long.

The step 2. will allow LFortran to be used interactively with production codes
right away (the production code will get compiled with GFortran, then one
"uses" any module in LFortran and functions/subroutines can be interactively
called, the module itself can use any GFortran supported feature, but the API
must fit into the subset that LFortran understands --- for large number of
applications simple functions/subroutines with array arguments are enough).
This will make LFortran usable for first users and one can always use GFortran
temporarily until LFortran supports the given feature.
We expect to be finished with the step 2. by the end of summer 2019, hopefully sooner.

Finally the step 3. will improve LFortran overall, allowing the first users
to contribute back, growing the community and making LFortran gradually useful
for more and more people.
