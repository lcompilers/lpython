#include <iostream>

#include <lfortran/config.h>
#include <lfortran/stacktrace.h>

int h()
{
    LFortran::show_stacktrace();
    std::vector<LFortran::StacktraceItem> d = LFortran::get_stacktrace_addresses();
    LFortran::get_local_addresses(d);
    LFortran::get_local_info(d);
    for (size_t i = 0; i < d.size(); i++) {
        std::cout << i << " ";
        std::cout << "pc: " << (void*) d[i].pc << " ";
        std::cout << "local_pc: " << (void*) d[i].local_pc << " ";
        std::cout << "binary: " << d[i].binary_filename << " ";
        std::cout << "fn: " << d[i].function_name << " ";
        std::cout << "line: " << d[i].line_number << " ";
        std::cout << "source: " << d[i].source_filename << " ";
        std::cout << std::endl;
    }
    return 42;
}

int g()
{
    return h();
}

int f()
{
    return g();
}

int main()
{
    int r;
    r = f();
    std::cout << r << std::endl;

    return 0;
}
