# Installation

## From a Tarball

This is the easiest way.

Install prerequisites and LFortran (works on both Linux and Mac):
```
conda create -n lfortran python=3.7 pytest llvmlite prompt_toolkit
conda activate lfortran
pip install antlr4-python3-runtime
tar xzf lfortran-0.1.tar.gz
cd lfortran-0.1
pip install .
```

Now the `lfortran` environment has the `lfort` compiler available.

Optional: run tests:
```
py.test --pyargs lfortran
```


## From Git

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
