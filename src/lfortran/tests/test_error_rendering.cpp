#include <tests/doctest.h>

#include <iostream>
#include <sstream>

#include <lfortran/diagnostics.h>
#include <lfortran/parser/parser.h>

namespace {

    // Strips the first character (\n)
    std::string S(const std::string &s) {
        return s.substr(1, s.size());
    }

}

using LFortran::diag::Diagnostic;
using LFortran::diag::Level;
using LFortran::diag::Stage;
using LFortran::diag::Label;
using LFortran::Location;
using LFortran::LocationManager;

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

    out = render_diagnostic(Diagnostic(
            "Error no label",
            Level::Warning, Stage::Semantic
        ), false
    );
    ref = S(R"""(
warning: Error no label
)""");
    CHECK(out == ref);
}

TEST_CASE("Error Render II") {
    std::string out, ref, input;
    Location loc;
    loc.first = 4;
    loc.last = 7;

    input = "One line text\n";
    LocationManager lm;
    lm.in_filename = "input";
    lm.get_newlines(input, lm.in_newlines);
    lm.out_start.push_back(0);
    lm.in_start.push_back(0);
    lm.in_start.push_back(input.size());
    lm.out_start.push_back(input.size());

    auto d = Diagnostic(
            "Error with label no message",
            Level::Error, Stage::Semantic, {
                Label("", {loc})
            }
        );
    out = render_diagnostic(d, input, lm, false, false);
    ref = S(R"""(
semantic error: Error with label no message
 --> input:1:5
  |
1 | One line text
  |     ^^^^ 
)""");
    CHECK(out == ref);



    Location loc2;
    loc2.first = 9;
    loc2.last = 12;
    auto d2 = Diagnostic(
            "Error with label and message",
            Level::Error, Stage::Semantic, {
                Label("label message", {loc, loc2})
            }
        );
    out = render_diagnostic(d2, input, lm, false, false);
    ref = S(R"""(
semantic error: Error with label and message
 --> input:1:5
  |
1 | One line text
  |     ^^^^ ^^^^ label message
)""");
    CHECK(out == ref);
}
