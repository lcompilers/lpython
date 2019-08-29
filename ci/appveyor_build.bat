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
call :check
set BUILD_TYPE=Debug
call :check
call %CONDA_INSTALL_LOCN%\Scripts\activate.bat

xonsh .\build0.xsh
call :check

set lfortran_version=0.0+git
call :check
pip install scikit-build
call :check
python setup.py sdist
call :check
pip uninstall -y scikit-build
call :check
tar xzf dist/lfortran-%lfortran_version%.tar.gz
call :check
cd lfortran-%lfortran_version%
call :check

mkdir test-bld
cd test-bld
cmake ..
call :check
cmake --build .
call :check
ctest --output-on-failure
call :check
cd ..

pip install -v .
call :check
cd ..

:: -----------------------------------------------------------------------------
:: End of script. We exit the script now.
goto :eof

:: Function definitions
:check
if errorlevel 1 (
    echo "Non zero exit code returned, aborting..."
    (goto) 2>nul & endlocal & exit /b %errorlevel%
)
