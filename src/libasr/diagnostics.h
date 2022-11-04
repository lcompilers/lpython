#ifndef LFORTRAN_DIAGNOSTICS_H
#define LFORTRAN_DIAGNOSTICS_H

#include <libasr/location.h>
#include <libasr/stacktrace.h>

namespace LFortran {

struct LocationManager;
struct CompilerOptions;

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

    Span(const Location &loc) : loc{loc} {}
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

    Label(const std::string &message, const std::vector<Location> &locations,
            bool primary=true) : primary{primary}, message{message} {
        for (auto &loc : locations) {
            spans.push_back(Span(loc));
        }
    }
};

/*
 * The diagnostic level is the type of the message.
 *
 * We can have errors, warnings, notes and help messages.
 */
enum Level {
    Error, Warning, Note, Help, Style
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
 * Errors have zero or more primary and zero or more secondary labels.
 * Help uses primary to show what should change.
 * Notes may not have any labels attached.
 *
 * The message describes the overall error/warning/note. Labels are used
 * to briefly but approachably describe what went wrong (primary label) and why
 * it happened (secondary label).
 *
 * A progression of error messages:
 *   * a message with no label
 *   * a message with a primary label, no attached message
 *   * a message with a primary label and attached message
 *   * a message with a primary label and attached message and secondary labels
 *   * ...
 * If there are labels attached, there must be at least one primary.
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
    std::vector<StacktraceItem> stacktrace = get_stacktrace_addresses();

    Diagnostic(const std::string &message, const Level &level,
        const Stage &stage) : level{level}, stage{stage}, message{message} {}

    Diagnostic(const std::string &message, const Level &level,
        const Stage &stage,
        const std::vector<Label> &labels
        ) : level{level}, stage{stage}, message{message}, labels{labels} {}
};

struct Diagnostics {
    std::vector<Diagnostic> diagnostics;

    std::string render(const std::string &input,
            const LocationManager &lm, const CompilerOptions &compiler_options);

    // Returns true iff diagnostics contains at least one error message
    bool has_error() const;

    void add(const Diagnostic &d) {
        diagnostics.push_back(d);
    }

    void message_label(const std::string &message,
            const std::vector<Location> &locations,
            const std::string &error_label,
            const Level &level,
            const Stage &stage
            ) {
        diagnostics.push_back(
            Diagnostic(message, level, stage, {Label(error_label, locations)})
        );
    }

    void semantic_warning_label(const std::string &message,
            const std::vector<Location> &locations, const std::string &error_label) {
        message_label(message, locations, error_label,
            Level::Warning, Stage::Semantic);
    }

    void semantic_error_label(const std::string &message,
            const std::vector<Location> &locations, const std::string &error_label) {
        message_label(message, locations, error_label,
            Level::Error, Stage::Semantic);
    }

    void tokenizer_warning_label(const std::string &message,
            const std::vector<Location> &locations, const std::string &error_label) {
        message_label(message, locations, error_label,
            Level::Warning, Stage::Tokenizer);
    }

    void parser_warning_label(const std::string &message,
            const std::vector<Location> &locations, const std::string &error_label) {
        message_label(message, locations, error_label,
            Level::Warning, Stage::Parser);
    }

    void codegen_warning_label(const std::string &message,
            const std::vector<Location> &locations, const std::string &error_label) {
        message_label(message, locations, error_label,
            Level::Warning, Stage::CodeGen);
    }

    void codegen_error_label(const std::string &message,
            const std::vector<Location> &locations, const std::string &error_label) {
        message_label(message, locations, error_label,
            Level::Error, Stage::CodeGen);
    }

    void tokenizer_style_label(const std::string &message,
            const std::vector<Location> &locations, const std::string &error_label) {
        message_label(message, locations, error_label,
            Level::Style, Stage::Tokenizer);
    }

    void parser_style_label(const std::string &message,
            const std::vector<Location> &locations, const std::string &error_label) {
        message_label(message, locations, error_label,
            Level::Style, Stage::Parser);
    }
};

std::string render_diagnostic_human(const Diagnostic &d, bool use_colors);
std::string render_diagnostic_short(const Diagnostic &d);

// Fills Diagnostic with span details and renders it
std::string render_diagnostic_human(Diagnostic &d, const std::string &input,
        const LocationManager &lm, bool use_colors, bool show_stacktrace);
std::string render_diagnostic_short(Diagnostic &d, const std::string &input,
        const LocationManager &lm);

} // namespace diag
} // namespace LFortran

#endif // LFORTRAN_DIAGNOSTICS_H
