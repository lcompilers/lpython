cmake ^
    -DCMAKE_INSTALL_PREFIX=%cd% ^
    -DCMAKE_GENERATOR_PLATFORM=x64 ^
    -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE ^
    .

cmake --build . --target install
