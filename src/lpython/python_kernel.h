#ifndef LFORTRAN_PYTHON_KERNEL_H
#define LFORTRAN_PYTHON_KERNEL_H

#include <libasr/config.h>
#include <string>

namespace LCompilers::LPython {

#ifdef HAVE_LFORTRAN_XEUS
    int run_kernel(const std::string &connection_filename);
#endif

} // namespace LCompilers::LFortran

#endif // LFORTRAN_PYTHON_KERNEL_H
