#include <iostream>
#include <chrono>
#include <lfortran/parser/parser.h>

using LFortran::parse;

int main(int argc, char *argv[])
{
    int N;
    N = 5000;
    std::string text;
    std::string t0 = "(a*z+3+2*x + 3*y - x/(z**2-4) - x**(y**z))";
    text.reserve(10000000);
    text = t0;
    std::cout << "Construct" << std::endl;
    for (int i = 0; i < N; i++) {
        text.append(" * " + t0);
    }
    std::cout << "Parse" << std::endl;
    auto t1 = std::chrono::high_resolution_clock::now();
    auto result = parse(text);
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
                     .count()
              << "ms" << std::endl;
    int c = count(*result);
    std::cout << "Count: " << c << std::endl;
    if (c == 45009) {
        return 0;
    } else {
        return 1;
    }
}
