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

TEST_CASE("Error Render: primary/secondary labels, single line") {
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

    // 3 Label 1 Span, one primary, two secondary
    d = Diagnostic(
            "Error with two labels and message",
            Level::Error, Stage::Semantic, {
                Label("label1 primary message", {loc1}),
                Label("label2 secondary message", {loc2}, false),
                Label("label3 secondary message", {loc3}, false)
            }
        );
    out = render_diagnostic(d, input, lm, false, false);
    ref = S(R"""(
semantic error: Error with two labels and message
 --> input:1:5
  |
1 | One line text
  |     ^^^^ label1 primary message
  |
1 | One line text
  |          ~~~~ label2 secondary message
  |
1 | One line text
  | ~~~ label3 secondary message
)""");
    CHECK(out == ref);

    // 3 Label 2 Spans, secondary, primary, secondary
    d = Diagnostic(
            "Error with three labels and message, two spans",
            Level::Error, Stage::Semantic, {
                Label("label1 secondary message", {loc1, loc2}, false),
                Label("label2 primary message", {loc3, loc2}),
                Label("label3 secondary message", {loc3}, false)
            }
        );
    out = render_diagnostic(d, input, lm, false, false);
    ref = S(R"""(
semantic error: Error with three labels and message, two spans
 --> input:1:5
  |
1 | One line text
  |     ~~~~ ~~~~ label1 secondary message
  |
1 | One line text
  | ^^^      ^^^^ label2 primary message
  |
1 | One line text
  | ~~~ label3 secondary message
)""");
    CHECK(out == ref);
}

TEST_CASE("Error Render: primary/secondary labels, multi line") {
    std::string out, ref, input;
    Location loc1, loc2, loc3;
    LocationManager lm;
    input = "One line text\nSecond line text\nThird line text\n";
    lm.in_filename = "input";
    lm.get_newlines(input, lm.in_newlines);
    lm.out_start.push_back(0);
    lm.in_start.push_back(0);
    lm.in_start.push_back(input.size());
    lm.out_start.push_back(input.size());

    loc1.first = 4; // 1 line
    loc1.last = 24; // 2 line
    loc2.first = 9; // 1 text
    loc2.last = 35; // 3 Third
    loc3.first = 0; // 1 One
    loc3.last = 2; // 3 Third

    // 1 Label 1 Span
    auto d = Diagnostic(
            "Error with label no message",
            Level::Error, Stage::Semantic, {
                Label("Multilines", {loc1})
            }
        );
    out = render_diagnostic(d, input, lm, false, false);
    ref = S(R"""(
semantic error: Error with label no message
 --> input:1:5 - 2:11
  |
1 |    One line text
  |        ^^^^^^^^^...
...
  |
2 |    Second line text
  | ...^^^^^^^^^^^ Multilines
)""");
    CHECK(out == ref);

    // 1 Label 2 Span
    d = Diagnostic(
            "Error with label, two spans",
            Level::Error, Stage::Semantic, {
                Label("Two spans", {loc1, loc2})
            }
        );
    out = render_diagnostic(d, input, lm, false, false);
    ref = S(R"""(
semantic error: Error with label, two spans
 --> input:1:5 - 2:11
  |
1 |    One line text
  |        ^^^^^^^^^...
...
  |
2 |    Second line text
  | ...^^^^^^^^^^^ Two spans
  |
1 |    One line text
  |             ^^^^...
...
  |
3 |    Third line text
  | ...^^^^^ Two spans
)""");
    CHECK(out == ref);

    // 1 Label 2 Span
    d = Diagnostic(
            "Error with label, two spans",
            Level::Error, Stage::Semantic, {
                Label("Two spans", {loc3, loc2})
            }
        );
    out = render_diagnostic(d, input, lm, false, false);
    ref = S(R"""(
semantic error: Error with label, two spans
 --> input:1:1
  |
1 | One line text
  | ^^^ Two spans
  |
1 |    One line text
  |             ^^^^...
...
  |
3 |    Third line text
  | ...^^^^^ Two spans
)""");
    CHECK(out == ref);
}
