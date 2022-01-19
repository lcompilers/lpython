#ifndef LFORTRAN_FORTRAN_KERNEL_H
#define LFORTRAN_FORTRAN_KERNEL_H

#include <libasr/config.h>

namespace LFortran {

#ifdef HAVE_LFORTRAN_XEUS
    int run_kernel(const std::string &connection_filename);
#endif

}

#endif // LFORTRAN_FORTRAN_KERNEL_H
