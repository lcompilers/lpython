#include <tests/doctest.h>

#include <iostream>
#include <sstream>

#include <lfortran/diagnostics.h>

namespace {

    // Strips the first character (\n)
    std::string S(const std::string &s) {
        return s.substr(1, s.size());
    }

}

using LFortran::diag::Diagnostic;
using LFortran::diag::Level;
using LFortran::diag::Stage;

TEST_CASE("Error Render I") {
    std::string out, ref;

    out = render_diagnostic(Diagnostic(
            "Error no label",
            Level::Error, Stage::Parser
        ), false
    );
    ref = S(R"""(
syntax error: Error no label
)""");
    CHECK(out == ref);


    out = render_diagnostic(Diagnostic(
            "Error no label",
            Level::Error, Stage::Semantic
        ), false
    );
    ref = S(R"""(
semantic error: Error no label
)""");
    CHECK(out == ref);
}
