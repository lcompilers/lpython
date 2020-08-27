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

// For registering SIGSEGV callbacks
#include <csignal>


// The following C headers are needed for some specific C functionality (see
// the comments), which is not available in C++:

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
#endif

namespace LFortran {


#ifdef HAVE_LFORTRAN_UNWIND

static _Unwind_Reason_Code unwind_callback(struct _Unwind_Context *context,
  void *vdata)
{
  std::vector<StacktraceItem> &d = *(std::vector<StacktraceItem> *)vdata;
  uintptr_t pc;
  pc = _Unwind_GetIP(context);
  if (pc != 0) {
    pc--;
    StacktraceItem i;
    i.pc = pc;
    d.push_back(i);
  }
  return _URC_NO_REASON;
}

#endif // HAVE_LFORTRAN_UNWIND



#ifdef HAVE_LFORTRAN_LINK

struct match_data {
  uintptr_t addr;

  std::string filename;
  uintptr_t addr_in_file;
};

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

// Fills in `local_pc` and `binary_filename` of `item`
void get_local_address(StacktraceItem &item)
{
    struct match_data match;
    match.addr = item.pc;
    // Iterate over all loaded shared libraries (see dl_iterate_phdr(3) -
    // Linux man page for more documentation)
    if (dl_iterate_phdr(shared_lib_callback, &match) == 0) {
      // `dl_iterate_phdr` returns the last value returned by our
      // `shared_lib_callback`. It will only be 0 if no shared library
      // (including the main program) contains the address `match.addr`. Given
      // that the addresses are collected from a stacktrace, this should only
      // happen if the stacktrace is somehow corrupted. In that case, we simply
      // abort here.
      std::cout << "The stack address was not found in any shared library or the main program, the stack is probably corrupted. Aborting." << std::endl;
      abort();
    }
    item.local_pc = match.addr_in_file;
    if (match.filename.length() > 0) {
      item.binary_filename = match.filename;
    } else {
      item.binary_filename = "/proc/self/exe";
    }
}


#endif // HAVE_LFORTRAN_LINK


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

/* This struct is used to pass information into process_section().
*/
struct line_data {
  asymbol **symbol_table;     /* Symbol table.  */
  uintptr_t addr;
  std::string filename;
  std::string function_name;
  unsigned int line;
  int line_found;
};


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
  bfd_vma offset = data->addr - section_vma;

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

void get_symbol_info(std::string binary_filename, uintptr_t addr,
  std::string &source_filename, std::string &function_name,
  int &line_number)
{
  line_data data;
  data.addr = addr;
  data.line_found = 0;
  // Initialize 'abfd' and do some sanity checks
  bfd *abfd;
  abfd = bfd_openr(binary_filename.c_str(), NULL);
  if (abfd == NULL) {
    std::cout << "Cannot open the binary file '" + binary_filename + "'\n";
    abort();
  }
  if (bfd_check_format(abfd, bfd_archive)) {
    std::cout << "Cannot get addresses from the archive '" + binary_filename + "'\n";
    abort();
  }
  char **matching;
  if (!bfd_check_format_matches(abfd, bfd_object, &matching)) {
    std::cout << "Unknown format of the binary file '" + binary_filename + "'\n";
    abort();
  }
  data.symbol_table = NULL;
  // This allocates the symbol_table:
  if (load_symbol_table(abfd, &data) == 1) {
    std::cout << "Failed to load the symbol table from '" + binary_filename + "'\n";
    abort();
  }
  // Loops over all sections and try to find the line
  bfd_map_over_sections(abfd, process_section, &data);
  // Deallocates the symbol table
  if (data.symbol_table != NULL) free(data.symbol_table);
  bfd_close(abfd);

  if (data.line_found) {
    std::string name = demangle_function_name(data.function_name);
    function_name = name;
    if (data.filename.length() > 0) {
      source_filename = data.filename;
      line_number = data.line;
    }
  }
}

#endif // HAVE_LFORTRAN_BFD



/* Return if given char is whitespace or not. */
bool is_whitespace_char(const char c)
{
  return c == ' ' || c == '\t';
}


/* Removes the leading whitespace from a string and returns the new
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




/* Returns a string of 2 lines for the function with address 'addr' in the file
   'file_name'.

   Example:

   File "/home/ondrej/repos/rcp/src/Teuchos_RCP.hpp", line 428, in Teuchos::RCP<A>::assert_not_null() const
   throw_null_ptr_error(typeName(*this));
*/
std::string addr2str(const StacktraceItem &i)
{
  std::ostringstream s;
  // Do the printing --- print as much information as we were able to
  // find out
  if (i.function_name == "") {
    // If we didn't find the line, at least print the address itself
    s << "  File unknown, address: 0x" << (long long unsigned int) i.local_pc;
  } else if (i.source_filename == "") {
      // The file is unknown (and data.line == 0 in this case), so the
      // only meaningful thing to print is the function name:
      s << color(style::dim) << "  File " + color(style::reset)
        << color(style::dim) << "unknown" + color(style::reset)
        << color(style::dim) << ", in " << i.function_name
        << color(style::reset);
  } else {
      // Nicely format the filename + function name + line
      s << color(style::dim) << "  File \"" << color(style::reset)
        << color(style::bold) << color(fg::magenta) << i.source_filename
        << color(fg::reset) << color(style::reset)
        << color(style::dim) << "\", line " << i.line_number
        << ", in " << i.function_name << color(style::reset);
      const std::string line_text = remove_leading_whitespace(
        read_line_from_file(i.source_filename, i.line_number));
      if (line_text != "") {
        s << "\n    " << line_text;
      }
  }
  s << "\n";
  return s.str();
}


/*
  Returns a std::string with the stacktrace corresponding to the
  list of addresses (of functions on the stack) in `d`.
*/
std::string stacktrace2str(const std::vector<StacktraceItem> &d, int skip)
{
  std::string full_stacktrace_str("Traceback (most recent call last):\n");

  // Loop over the stack
  for (int i=d.size()-1; i >= skip; i--) {
    full_stacktrace_str += addr2str(d[i]);
  }

  return full_stacktrace_str;
}


void loc_segfault_callback_print_stack(int /* sig_num */)
{
  std::cerr << get_stacktrace(3);
  std::cerr << "Segfault: Signal SIGSEGV (segmentation fault) received\n";
  exit(1);
}


void loc_abort_callback_print_stack(int /* sig_num */)
{
  std::cerr << get_stacktrace(3);
  std::cerr << "Abort: Signal SIGABRT (abort) received\n\n";
}

void print_stack_on_segfault()
{
  signal(SIGSEGV, loc_segfault_callback_print_stack);
  signal(SIGABRT, loc_abort_callback_print_stack);
}

std::string get_stacktrace(int skip)
{
  std::vector<StacktraceItem> d = get_stacktrace_addresses();
  get_local_addresses(d);
  get_local_info(d);
  return stacktrace2str(d, skip);
}


void show_stacktrace()
{
  std::cout << get_stacktrace(3);
}


std::vector<StacktraceItem> get_stacktrace_addresses()
{
  std::vector<StacktraceItem> d;
#ifdef HAVE_LFORTRAN_UNWIND
  _Unwind_Backtrace(unwind_callback, &d);
#endif
  return d;
}

void get_local_addresses(std::vector<StacktraceItem> &d)
{
  for (size_t i=0; i < d.size(); i++) {
#ifdef HAVE_LFORTRAN_LINK
    get_local_address(d[i]);
#endif
  }
}


void get_local_info(std::vector<StacktraceItem> &d)
{
#ifdef HAVE_LFORTRAN_BFD
  bfd_init();
#endif
  for (size_t i=0; i < d.size(); i++) {
#ifdef HAVE_LFORTRAN_BFD
    get_symbol_info(d[i].binary_filename, d[i].local_pc,
      d[i].source_filename, d[i].function_name, d[i].line_number);
#endif
  }
}

} // namespace LFortran
