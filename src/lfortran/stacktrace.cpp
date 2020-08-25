#include <lfortran/stacktrace.h>
#include <lfortran/config.h>
#include <lfortran/colors.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// free() and abort() functions
#include <cstdlib>

// For handling variable number of arguments using va_start/va_end functions
#include <cstdarg>

// For registering SIGSEGV callbacks
#include <csignal>


// The following C headers are needed for some specific C functionality (see
// the comments), which is not available in C++:

#ifdef HAVE_LFORTRAN_EXECINFO
// backtrace() function for retrieving the stacktrace
#  include <execinfo.h>
#endif

#ifdef HAVE_LFORTRAN_UNWIND
// For _Unwind_Backtrace() function
#  include <unwind.h>
#endif

#if defined(HAVE_LFORTRAN_DEMANGLE)
// For demangling function names
#  include <cxxabi.h>
#endif

#ifdef HAVE_LFORTRAN_LINK
// For dl_iterate_phdr() functionality
#  include <link.h>
#endif

#ifdef HAVE_LFORTRAN_BFD
// For bfd_* family of functions for loading debugging symbols from the binary
// This is the only nonstandard header file and the binary needs to be linked
// with "-lbfd".
#  include <bfd.h>
#else
typedef long long unsigned bfd_vma;
#endif

using LFortran::color;
using LFortran::style;
using LFortran::fg;

namespace {

/* This struct is used to pass information between
   addr2str() and process_section().
*/
struct line_data {
#ifdef HAVE_LFORTRAN_BFD
  asymbol **symbol_table;     /* Symbol table.  */
#endif
  bfd_vma addr;
  std::string filename;
  std::string function_name;
  unsigned int line;
  int line_found;
};


/* Return if given char is whitespace or not. */
bool is_whitespace_char(const char c)
{
  return c == ' ' || c == '\t';
}


/* Removes the leading whitespace from a string and returnes the new
 * string.
 */
std::string remove_leading_whitespace(const std::string &str)
{
  if (str.length() && is_whitespace_char(str[0])) {
    int first_nonwhitespace_index = 0;
    for (int i = 0; i < static_cast<int>(str.length()); ++i) {
      if (!is_whitespace_char(str[i])) {
        first_nonwhitespace_index = i;
        break;
      }
    }
    return str.substr(first_nonwhitespace_index);
  }
  return str;
}


/* Reads the 'line_number'th line from the file filename. */
std::string read_line_from_file(std::string filename, unsigned int line_number)
{
  std::ifstream in(filename.c_str());
  if (!in.is_open()) {
    return "";
  }
  if (line_number == 0) {
    return "Line number must be positive";
  }
  unsigned int n = 0;
  std::string line;
  while (n < line_number) {
    if (in.eof())
      return "Line not found";
    getline(in, line);
    n += 1; // loop update
  }
  return line;
}

/* Demangles the function name if needed (if the 'name' is coming from C, it
   doesn't have to be demangled, if it's coming from C++, it needs to be).

   Makes sure that it ends with (), which is automatic in C++, but it has to be
   added by hand in C.
*/
std::string demangle_function_name(std::string name)
{
  std::string s;

  if (name.length() == 0) {
    s = "??";
  } else {
    char *d = 0;
#if defined(HAVE_LFORTRAN_DEMANGLE)
    int status = 0;
    d = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
#endif
    if (d) {
      s = d;
      free(d);
    } else {
      s = name + "()";
    }
  }

  return s;
}


#ifdef HAVE_LFORTRAN_BFD


/* Look for an address in a section.  This is called via
   bfd_map_over_sections over all sections in abfd.

   If the correct line is found, store the result in 'data' and set
   data->line_found, so that subsequent calls to process_section exit
   immediately.
*/
void process_section(bfd *abfd, asection *section, void *_data)
{
  line_data *data = (line_data*)_data;
  if (data->line_found) {
    // If we already found the line, exit
    return;
  }
  if ((bfd_get_section_flags(abfd, section) & SEC_ALLOC) == 0) {
    return;
  }

  bfd_vma section_vma = bfd_get_section_vma(abfd, section);
  if (data->addr < section_vma) {
    // If the addr lies above the section, exit
    return;
  }

  bfd_size_type section_size = bfd_section_size(abfd, section);
  if (data->addr >= section_vma + section_size) {
    // If the addr lies below the section, exit
    return;
  }

  // Calculate the correct offset of our line in the section
  bfd_vma offset = data->addr - section_vma - 1;

  // Finds the line corresponding to the offset

  const char *filename=NULL, *function_name=NULL;
  data->line_found = bfd_find_nearest_line(abfd, section, data->symbol_table,
    offset, &filename, &function_name, &data->line);

  if (filename == NULL)
    data->filename = "";
  else
    data->filename = filename;

  if (function_name == NULL)
    data->function_name = "";
  else
    data->function_name = function_name;
}


/* Loads the symbol table into 'data->symbol_table'.  */
int load_symbol_table(bfd *abfd, line_data *data)
{
  if ((bfd_get_file_flags(abfd) & HAS_SYMS) == 0)
    // If we don't have any symbols, return
    return 0;

  void **symbol_table_ptr = reinterpret_cast<void **>(&data->symbol_table);
  long n_symbols;
  unsigned int symbol_size;
  n_symbols = bfd_read_minisymbols(abfd, false, symbol_table_ptr, &symbol_size);
  if (n_symbols == 0) {
    // If the bfd_read_minisymbols() already allocated the table, we need
    // to free it first:
    if (data->symbol_table != NULL)
      free(data->symbol_table);
    // dynamic
    n_symbols = bfd_read_minisymbols(abfd, true, symbol_table_ptr, &symbol_size);
  }

  if (n_symbols < 0) {
    // bfd_read_minisymbols() failed
    return 1;
  }

  return 0;
}


#endif // HAVE_LFORTRAN_BFD


/* Returns a string of 2 lines for the function with address 'addr' in the file
   'file_name'.

   Example:

   File "/home/ondrej/repos/rcp/src/Teuchos_RCP.hpp", line 428, in Teuchos::RCP<A>::assert_not_null() const
   throw_null_ptr_error(typeName(*this));
*/
std::string addr2str(std::string file_name, bfd_vma addr)
{
#ifdef HAVE_LFORTRAN_BFD
  // Initialize 'abfd' and do some sanity checks
  bfd *abfd;
  abfd = bfd_openr(file_name.c_str(), NULL);
  if (abfd == NULL)
    return "Cannot open the binary file '" + file_name + "'\n";
  if (bfd_check_format(abfd, bfd_archive))
    return "Cannot get addresses from the archive '" + file_name + "'\n";
  char **matching;
  if (!bfd_check_format_matches(abfd, bfd_object, &matching))
    return "Unknown format of the binary file '" + file_name + "'\n";
  line_data data;
  data.addr = addr;
  data.symbol_table = NULL;
  data.line_found = false;
  // This allocates the symbol_table:
  if (load_symbol_table(abfd, &data) == 1)
    return "Failed to load the symbol table from '" + file_name + "'\n";
  // Loops over all sections and try to find the line
  bfd_map_over_sections(abfd, process_section, &data);
  // Deallocates the symbol table
  if (data.symbol_table != NULL) free(data.symbol_table);
  bfd_close(abfd);
#else
  line_data data;
  data.line_found = 0;
#endif

  std::ostringstream s;
  // Do the printing --- print as much information as we were able to
  // find out
  if (!data.line_found) {
    // If we didn't find the line, at least print the address itself
    s << "  File unknown, address: 0x" << (long long unsigned int) addr;
  } else {
    std::string name = demangle_function_name(data.function_name);
    if (data.filename.length() > 0) {
      // Nicely format the filename + function name + line
      s << color(style::dim) << "  File \"" << color(style::reset)
        << color(style::bold) << color(fg::magenta) << data.filename
        << color(fg::reset) << color(style::reset)
        << color(style::dim) << "\", line " << data.line << ", in " << name
        << color(style::reset);
      const std::string line_text = remove_leading_whitespace(
        read_line_from_file(data.filename, data.line));
      if (line_text != "") {
        s << "\n    " << line_text;
      }
    } else {
      // The file is unknown (and data.line == 0 in this case), so the
      // only meaningful thing to print is the function name:
      s << color(style::dim) << "  File " + color(style::reset)
        << color(style::dim) << "unknown" + color(style::reset)
        << color(style::dim) << ", in " << name << color(style::reset);
    }
  }
  s << "\n";
  return s.str();
}

struct match_data {
  bfd_vma addr;

  std::string filename;
  bfd_vma addr_in_file;
};


#ifdef HAVE_LFORTRAN_LINK


/* Tries to find the 'data.addr' in the current shared lib (as passed in
   'info'). If it succeeds, returns (in the 'data') the full path to the shared
   lib and the local address in the file.
*/
int shared_lib_callback(struct dl_phdr_info *info,
  size_t /* size */, void *_data)
{
  struct match_data *data = (struct match_data *)_data;
  for (int i=0; i < info->dlpi_phnum; i++) {
    if (info->dlpi_phdr[i].p_type == PT_LOAD) {
      ElfW(Addr) min_addr = info->dlpi_addr + info->dlpi_phdr[i].p_vaddr;
      ElfW(Addr) max_addr = min_addr + info->dlpi_phdr[i].p_memsz;
      if ((data->addr >= min_addr) && (data->addr < max_addr)) {
        data->filename = info->dlpi_name;
        data->addr_in_file = data->addr - info->dlpi_addr;
        // We found a match, return a non-zero value
        return 1;
      }
    }
  }
  // We didn't find a match, return a zero value
  return 0;
}


#endif // HAVE_LFORTRAN_LINK

// Class for creating a safe C++ interface to the raw void** stacktrace
// pointers, that we get from the backtrace() libc function. We make a copy of
// the addresses, so the caller can free the memory. We use std::vector to
// store the addresses internally, but this can be changed.
class StacktraceAddresses {
  std::vector<bfd_vma> stacktrace_buffer;
  int impl_stacktrace_depth;
public:
  StacktraceAddresses(void *const *_stacktrace_buffer, int _size, int _impl_stacktrace_depth)
    : impl_stacktrace_depth(_impl_stacktrace_depth)
    {
      for (int i=0; i < _size; i++)
        stacktrace_buffer.push_back((bfd_vma) _stacktrace_buffer[i]);
    }
  bfd_vma get_address(int i) const {
    return this->stacktrace_buffer[i];
  }
  int get_size() const {
    return this->stacktrace_buffer.size();
  }
  int get_impl_stacktrace_depth() const {
    return this->impl_stacktrace_depth;
  }
};


/*
  Returns a std::string with the stacktrace corresponding to the
  list of addresses (of functions on the stack) in 'buffer'.

  It converts addresses to filenames, line numbers, function names and the
  line text.
*/
std::string stacktrace2str(const StacktraceAddresses &stacktrace_addresses)
{
  int stack_depth = stacktrace_addresses.get_size() - 1;

  std::string full_stacktrace_str("Traceback (most recent call last):\n");

#ifdef HAVE_LFORTRAN_BFD
  bfd_init();
#endif
  // Loop over the stack
  const int stack_depth_start = stack_depth;
  const int stack_depth_end = stacktrace_addresses.get_impl_stacktrace_depth();
  for (int i=stack_depth_start; i >= stack_depth_end; i--) {
    // Iterate over all loaded shared libraries (see dl_iterate_phdr(3) -
    // Linux man page for more documentation)
    struct match_data match;
    match.addr = stacktrace_addresses.get_address(i);
#ifdef HAVE_LFORTRAN_BFD
    if (dl_iterate_phdr(shared_lib_callback, &match) == 0)
      return "dl_iterate_phdr() didn't find a match\n";
#else
    match.filename = "";
    match.addr_in_file = match.addr;
#endif

    if (match.filename.length() > 0) {
      // This happens for shared libraries (like /lib/libc.so.6, or any
      // other shared library that the project uses). 'match.filename'
      // then contains the full path to the .so library.
      full_stacktrace_str += addr2str(match.filename, match.addr_in_file);
    } else {
      // The 'addr_in_file' is from the current executable binary, that
      // one can find at '/proc/self/exe'. So we'll use that.
      full_stacktrace_str += addr2str("/proc/self/exe", match.addr_in_file);
    }
  }

  return full_stacktrace_str;
}


void loc_segfault_callback_print_stack(int /* sig_num */)
{
  std::cerr << LFortran::get_stacktrace(1);
  std::cerr << "Segfault: Signal SIGSEGV (segmentation fault) received\n";
  exit(1);
}


void loc_abort_callback_print_stack(int /* sig_num */)
{
  std::cerr << LFortran::get_stacktrace(1);
  std::cerr << "Abort: Signal SIGABRT (abort) received\n\n";
}

#ifdef HAVE_LFORTRAN_UNWIND
struct unwind_callback_data {
  std::vector<void*> stacktrace;
};

static _Unwind_Reason_Code unwind_callback(struct _Unwind_Context *context,
  void *vdata)
{
  unwind_callback_data *data = (unwind_callback_data *) vdata;
  uintptr_t pc;
  pc = _Unwind_GetIP(context);
  data->stacktrace.push_back((void*)pc);
  return _URC_NO_REASON;
}
#endif


StacktraceAddresses get_stacktrace_addresses(int impl_stacktrace_depth)
{
  void **stack=nullptr;
  size_t stacktrace_size=0;
#ifdef HAVE_LFORTRAN_EXECINFO
  const int STACKTRACE_ARRAY_SIZE = 1024;
  void *stacktrace_array[STACKTRACE_ARRAY_SIZE];
  stacktrace_size = backtrace(stacktrace_array, STACKTRACE_ARRAY_SIZE);
  stack = stacktrace_array;
#else
#  ifdef HAVE_LFORTRAN_UNWIND
  unwind_callback_data data;
  _Unwind_Backtrace(unwind_callback, &data);
  stack = &data.stacktrace[0];
  stacktrace_size = data.stacktrace.size()-1;
#  endif
#endif
  return StacktraceAddresses(stack, stacktrace_size, impl_stacktrace_depth+1);
}


} // Unnamed namespace


// Public functions


std::string LFortran::get_stacktrace(int impl_stacktrace_depth)
{
  StacktraceAddresses addresses =
    get_stacktrace_addresses(impl_stacktrace_depth+1);
  return stacktrace2str(addresses);
}


void LFortran::show_stacktrace()
{
  const int impl_stacktrace_depth=1;
  std::cout << LFortran::get_stacktrace(impl_stacktrace_depth);
}


void LFortran::print_stack_on_segfault()
{
  signal(SIGSEGV, loc_segfault_callback_print_stack);
  signal(SIGABRT, loc_abort_callback_print_stack);
}
