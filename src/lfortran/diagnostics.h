#ifndef LFORTRAN_DIAGNOSTICS_H
#define LFORTRAN_DIAGNOSTICS_H

#include <lfortran/parser/location.h>

namespace LFortran {

namespace diag {

struct Span {
    Location loc; // Linear location (span), must be filled out

    // Later the `loc` is used to populate these:
    // Converted to line+columns
    uint32_t first_line, first_column, last_line, last_column;
    // Filename:
    std::string filename;
    // Lines of source code from first_line to last_line
    std::vector<std::string> source_code;
};

/*
 * Labels can be primary or secondary.
 *
 * An optional message can be attached to the label.
 *
 *   * Primary: brief, but approachable description of *what* went wrong
 *   * Secondary: description of *why* the error happened
 *
 * Primary label uses ^^^, secondary uses ~~~ (or ---)
 *
 * There is one or more spans (Locations) attached to a label.
 *
 * Colors:
 *
 *   * Error message: primary is red, secondary is blue
 *   * Warning message: primary is yellow
 */
struct Label {
    bool primary; // primary or secondary label
    std::string message; // message attached to the label
    std::vector<Span> spans; // one or more spans
};

/*
 * The diagnostic level is the type of the message.
 *
 * We can have errors, warnings, notes and help messages.
 */
enum Level {
    Error, Warning, Note, Help
};

/*
 * Which stage of the compiler the error is coming from
 */
enum Stage {
    CPreprocessor, Prescanner, Tokenizer, Parser, Semantic, ASRPass, CodeGen
};

/*
 * A diagnostic message has a level and message and labels.
 *
 * Errors have one or more primary and zero or more secondary labels.
 * Help uses primary to show what should change.
 * Notes may not have any labels attached.
 *
 * The message describes the overall error/warning/note. Labels are used
 * to briefly but approachably describe what went wrong (primary label) and why
 * it happened (secondary label).
 *
 * The main diagnostic message is the parent. It can have children that can
 * attach notes, help, etc. to the main error or warning message.
 */
struct Diagnostic {
    Level level;
    Stage stage;
    std::string message;
    std::vector<Label> labels;
    std::vector<Diagnostic> children;
};

std::string render_diagnostic(const Diagnostic &d, bool use_colors);

} // namespace diag
} // namespace LFortran

#endif // LFORTRAN_DIAGNOSTICS_H
