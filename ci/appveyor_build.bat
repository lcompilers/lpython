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

python grammar\asdl_py.py
call :check

python grammar\asdl_py.py grammar\ASR.asdl lfortran\asr\asr.py ..ast.utils
call :check

cd grammar
call :check
java -cp antlr-4.7-complete.jar org.antlr.v4.Tool -Dlanguage=Python3 -no-listener -visitor fortran.g4 -o ..\lfortran\parser
call :check
cd ..
call :check


set CONDA_INSTALL_LOCN=C:\\Miniconda37-x64
call :check
set BUILD_TYPE=Debug
call :check
call %CONDA_INSTALL_LOCN%\Scripts\activate.bat
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
pip install -v .
call :check
cd ..
call :check

:: -----------------------------------------------------------------------------
:: End of script. We exit the script now.
goto :eof

:: Function definitions
:check
if errorlevel 1 (
    echo "Non zero exit code returned, aborting..."
    (goto) 2>nul & endlocal & exit /b %errorlevel%
)
