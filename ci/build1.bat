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
call %CONDA_INSTALL_LOCN%\Scripts\activate.bat
call :check
cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_INSTALL_PREFIX=%cd% -DBUILD_SHARED_LIBS=OFF .
call :check
cmake --build . --config %BUILD_TYPE% --target install
call :check
cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_INSTALL_PREFIX=%cd% -DBUILD_SHARED_LIBS=ON .
call :check
cmake --build . --config %BUILD_TYPE% --target install
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
