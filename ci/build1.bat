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

set CFLAGS=-Wall -g -fPIC
call :check
set PATH=C:\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin;%PATH%
call :check
gcc %CFLAGS% -o lfort_intrinsics.o -c lfort_intrinsics.c
call :check
ar rcs liblfortran.a lfort_intrinsics.o
call :check
gcc -shared -o liblfortran.so lfort_intrinsics.o
call :check
mkdir share\lfortran\lib
call :check
xcopy liblfortran.a share\lfortran\lib\
call :check
xcopy liblfortran.so share\lfortran\lib\
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
