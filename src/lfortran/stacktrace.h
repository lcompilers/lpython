#ifndef LFORTRAN_STACKTRACE_H
#define LFORTRAN_STACKTRACE_H

#include <string>

namespace LFortran {

/* Returns the current stacktrace as a string.
 *
 * impl_stacktrace_depth ... The stacktrace depth to remove from the stacktrace
 *     printout to avoid showing users implementation functions in the
 *     stacktrace.
 */
std::string get_stacktrace(int impl_stacktrace_depth=0);

// Prints the current stacktrace to stdout.
void show_stacktrace();

//  Prints the current stacktrace to stdout on segfault.
void print_stack_on_segfault();

} // namespace LFortran

#endif // LFORTRAN_STACKTRACE_H
