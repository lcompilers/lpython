# LFortran

[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gl/lfortran%2Fweb%2Flfortran-binder/master?filepath=Demo.ipynb)
[![project chat](https://img.shields.io/badge/zulip-join_chat-brightgreen.svg)](https://lfortran.zulipchat.com/)

LFortran is a modern open-source (BSD licensed) interactive Fortran compiler
built on top of LLVM. It can execute user's code interactively to allow
exploratory work (much like Python, MATLAB or Julia) as well as compile to
binaries with the goal to run user's code on modern architectures such as
multi-core CPUs and GPUs.

Website: https://lfortran.org/

# Documentation

All documentation, installation instructions, motivation, design, ... is
available at:

https://docs.lfortran.org/

Which is generated using the files in the `doc` directory.


# Development

The main development repository is at GitLab:

https://gitlab.com/lfortran/lfortran

Please use it to open issues or send merge requests.

You can also chat with us on Zulip ([![project chat](https://img.shields.io/badge/zulip-join_chat-brightgreen.svg)](https://lfortran.zulipchat.com/)).

CI status on master:

Linux: https://gitlab.com/lfortran/lfortran/pipelines

macOS:
[![Build Status](https://dev.azure.com/lfortran/lfortran/_apis/build/status/lfortran?branchName=master)](https://dev.azure.com/lfortran/lfortran/_build/latest?definitionId=1&branchName=master)

Windows:
[![Build status](https://ci.appveyor.com/api/projects/status/qeaanx87eypihj8p/branch/master?svg=true)](https://ci.appveyor.com/project/certik/lfortran-ts83e/branch/master)


We maintain an official GitHub read-only
[mirror](https://github.com/lfortran/lfortran)
(please do not send pull
requests there, use our GitLab repository instead).
