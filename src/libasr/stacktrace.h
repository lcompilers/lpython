#ifndef LFORTRAN_STACKTRACE_H
#define LFORTRAN_STACKTRACE_H

#include <cstdint>
#include <string>
#include <vector>

namespace LCompilers {

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

// Path to the binary executable
extern std::string binary_executable_path;

struct StacktraceItem
{
  // Always found
  uintptr_t pc;

  // The following two are either both found, or not found
  uintptr_t local_pc=0; // 0 if not found
  std::string binary_filename; // "" if not found

  // This can be found or not
  std::string function_name; // "" if not found

  // The following two are either both found, or not found
  std::string source_filename; // "" if not found
  int line_number=-1; // -1 if not found
};

// Returns the stacktrace, fills in the `pc` member
std::vector<StacktraceItem> get_stacktrace_addresses();

// Fills in the `local_pc` and `binary_filename` members
void get_local_addresses(std::vector<StacktraceItem> &d);

// Fills in the `function_name` if available, and if so, also fills in
// `source_filename` and `line_number` if available
void get_local_info(std::vector<StacktraceItem> &d);

// Converts the information stored in `d` into a string
std::string stacktrace2str(const std::vector<StacktraceItem> &d,
    int skip);

// Returns line number information from address
void address_to_line_number(const std::vector<std::string> &filenames,
          const std::vector<uint64_t> &addresses,
          uintptr_t address,
          std::string &filename,
          int &line_number);

std::string error_stacktrace(const std::vector<StacktraceItem> &stacktrace);

} // namespace LCompilers

#endif // LFORTRAN_STACKTRACE_H
