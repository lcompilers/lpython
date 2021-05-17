To compile with LFortran:

    FC=lfortran cmake .

To choose a backend:

    FC=lfortran FFLAGS="--backend=cpp" cmake .
    FC=lfortran FFLAGS="--backend=llvm" cmake .
