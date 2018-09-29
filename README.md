# LFortran

LFortran is an interactive Fortran compiler built on top of LLVM.

## How to compile

This works both on Linux and a Mac:

```
conda create -n lfortran python=3.7 pytest llvmlite prompt_toolkit
conda activate lfortran
pip install antlr4-python3-runtime
```

Install Java and then ANTLR, say, into `~/ext`:
```
export CLASSPATH="$HOME/ext/antlr-4.7-complete.jar:$CLASSPATH"
```

Build:
```
./build.sh
```

Run tests:

```
py.test
```

Run prompt:
```
./prompt
```
