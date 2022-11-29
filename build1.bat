cmake ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DWITH_LLVM=yes ^
    -DLPYTHON_BUILD_ALL=yes ^
    -DWITH_STACKTRACE=no ^
    -DCMAKE_PREFIX_PATH="%CONDA_PREFIX%" ^
    -DCMAKE_INSTALL_PREFIX=%cd%/inst ^
    . -G "Ninja"

cmake --build . --config Release -j8 --target install
