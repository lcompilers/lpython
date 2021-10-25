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

TEST_CASE("Error Render: no labels") {
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

TEST_CASE("Error Render: only primary labels") {
    // All combinations of primary error labels
    std::string out, ref, input;
    Location loc1, loc2, loc3;
    LocationManager lm;
    input = "One line text\n";
    lm.in_filename = "input";
    lm.get_newlines(input, lm.in_newlines);
    lm.out_start.push_back(0);
    lm.in_start.push_back(0);
    lm.in_start.push_back(input.size());
    lm.out_start.push_back(input.size());

    loc1.first = 4;
    loc1.last = 7;
    loc2.first = 9;
    loc2.last = 12;
    loc3.first = 0;
    loc3.last = 2;

    // 1 Label 1 Span
    auto d = Diagnostic(
            "Error with label no message",
            Level::Error, Stage::Semantic, {
                Label("", {loc1})
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


    // 1 Label 2 Spans
    d = Diagnostic(
            "Error with label and message",
            Level::Error, Stage::Semantic, {
                Label("label message", {loc1, loc2})
            }
        );
    out = render_diagnostic(d, input, lm, false, false);
    ref = S(R"""(
semantic error: Error with label and message
 --> input:1:5
  |
1 | One line text
  |     ^^^^ ^^^^ label message
)""");
    CHECK(out == ref);

    // 1 Label 3 Spans
    d = Diagnostic(
            "Error with label and message",
            Level::Error, Stage::Semantic, {
                Label("label message", {loc1, loc2, loc3})
            }
        );
    out = render_diagnostic(d, input, lm, false, false);
    ref = S(R"""(
semantic error: Error with label and message
 --> input:1:5
  |
1 | One line text
  |     ^^^^ ^^^^ label message
  |
1 | One line text
  | ^^^ label message
)""");
    CHECK(out == ref);

    // 2 Label 1 Span
    d = Diagnostic(
            "Error with two labels and message",
            Level::Error, Stage::Semantic, {
                Label("label1 message", {loc1}),
                Label("label2 message", {loc2})
            }
        );
    out = render_diagnostic(d, input, lm, false, false);
    ref = S(R"""(
semantic error: Error with two labels and message
 --> input:1:5
  |
1 | One line text
  |     ^^^^ label1 message
  |
1 | One line text
  |          ^^^^ label2 message
)""");
    CHECK(out == ref);

    // 3 Label 1 Span
    d = Diagnostic(
            "Error with two labels and message",
            Level::Error, Stage::Semantic, {
                Label("label1 message", {loc1}),
                Label("label2 message", {loc2}),
                Label("label3 message", {loc3})
            }
        );
    out = render_diagnostic(d, input, lm, false, false);
    ref = S(R"""(
semantic error: Error with two labels and message
 --> input:1:5
  |
1 | One line text
  |     ^^^^ label1 message
  |
1 | One line text
  |          ^^^^ label2 message
  |
1 | One line text
  | ^^^ label3 message
)""");
    CHECK(out == ref);
}
