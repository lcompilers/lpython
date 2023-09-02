#ifndef LFORTRAN_API
#define LFORTRAN_API
#endif

#include <runtime/lfortran_intrinsics.h>
#include <random>

extern "C" {
    LFORTRAN_API double _lfortran_random() {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        std::uniform_real_distribution<> dist(0, 1);

        return dist(gen);
    }
}
