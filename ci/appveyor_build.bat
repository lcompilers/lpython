:: Good documentation for Windows batch files:
:: https://ss64.com/nt/
::
:: Write the script between the two horizontal lines (`--------------`).
:: Put `call :check` after each command. This will ensure that if the command
:: fails, then the script will abort immediately with a nonzero exit code.
:: Doing that makes it equivalent to `set -e` in Bash.
::
:: Enable echo (equivalent of `set -x` in Bash)
@echo on
:: Inherit all current variables from the master environment
setlocal
::
:: -----------------------------------------------------------------------------


set CONDA_INSTALL_LOCN=C:\\Miniconda37-x64
set BUILD_TYPE=Debug
call %CONDA_INSTALL_LOCN%\Scripts\activate.bat

xonsh build0.xsh

set lfortran_version=0.0+git
pip install scikit-build
python setup.py sdist
pip uninstall -y scikit-build
tar xzf dist/lfortran-%lfortran_version%.tar.gz
cd lfortran-%lfortran_version%

mkdir test-bld
cd test-bld
cmake ..
cmake --build .
ctest --output-on-failure
cd ..

pip install -v .
cd ..
