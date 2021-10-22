#include <iostream>
#include <fstream>
#include <chrono>

#include <fmt/core.h>

#include <lfortran/parser/parser.h>
#include <lfortran/pickle.h>

std::string construct_fortran(size_t N) {
    std::string sub_template = R"(
subroutine g{num1}(x)
    integer, intent(inout) :: x
    integer :: i
    x = 0
    do i = {num1}, {num2}
        x = x+i
    end do
end subroutine
)";

    std::string call_template = R"(call g{num1}(c)
)";

    std::string text;
    std::string st0 = R"(program bench3
implicit none
integer :: c
c = 0
)";
    std::string st1 = R"(
print *, c

contains
)";
    std::string st3 = R"(
end program
)";
    text.reserve(2250042);
    text = st0;
    for (size_t i = 0; i < N; i++) {
        text += fmt::format(call_template,
                fmt::arg("num1", i+1)
                );
    }
    text += st1;
    for (size_t i = 0; i < N; i++) {
        text += fmt::format(sub_template,
                fmt::arg("num1", i+1),
                fmt::arg("num2", i+10)
                );
    }
    text += st3;
    return text;
}

std::string construct_c(size_t N) {
    std::string sub_template = R"(
void g{num1}(int *x)
{{
    int i;
    *x = 0;
    for (i = {num1}; i <= {num2}; i++) {{
        *x = *x+i;
    }}
}}
)";

    std::string call_template = R"(    g{num1}(&c);
)";

    std::string text;
    std::string st0 = R"(
int main()
{
    int c;
    c = 0;
)";
    std::string st1 = R"(
    printf("%d\n", c);
}
)";
    text.reserve(2250042);
    text = "#include <stdio.h>\n";
    for (size_t i = 0; i < N; i++) {
        text += fmt::format(sub_template,
                fmt::arg("num1", i+1),
                fmt::arg("num2", i+10)
                );
    }
    text += st0;
    for (size_t i = 0; i < N; i++) {
        text += fmt::format(call_template,
                fmt::arg("num1", i+1)
                );
    }
    text += st1;
    return text;
}

int main()
{
    int N;
    N = 10000;

    std::string text = construct_fortran(N);
    {
        std::ofstream file;
        file.open("bench3.f90");
        file << text;
    }
    {
        std::string text = construct_c(N);
        std::ofstream file;
        file.open("bench3.c");
        file << text;
    }

    Allocator al(64*1024*1024); // The actual size is 31,600,600
    LFortran::diag::Diagnostics diagnostics;
    std::cout << "Parse" << std::endl;
    auto t1 = std::chrono::high_resolution_clock::now();
    auto res = LFortran::parse(al, text, diagnostics);
    auto t2 = std::chrono::high_resolution_clock::now();

    auto result = LFortran::TRY(res);
    std::string p = LFortran::pickle(*result);
    std::cout << "Number of units: " << result->n_items << std::endl;

    std::cout << "Parsing: " <<
        std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
        << "ms" << std::endl;
    std::cout << "String size (bytes):      " << text.size() << std::endl;
    std::cout << "Allocator usage (bytes): " << al.size_current() << std::endl;
    std::cout << "Allocator chunks: " << al.num_chunks() << std::endl;

    return 0;
}
