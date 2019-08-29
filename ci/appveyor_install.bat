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

call %CONDA_INSTALL_LOCN%\Scripts\activate.bat
call :check

call conda config --set always_yes yes --set changeps1 no
call :check
call conda info -a
call :check
call conda update -q conda
call :check
call conda install -c conda-forge python=3.7 re2c m2-bison xonsh
call :check

cd grammar
call :check
curl -O https://www.antlr.org/download/antlr-4.7-complete.jar
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
