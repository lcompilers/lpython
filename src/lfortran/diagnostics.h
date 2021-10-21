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

    static Diagnostic semantic_error(const std::string &message, const Location &loc) {
        diag::Span s;
        s.loc = loc;
        diag::Label l;
        l.primary = true;
        l.message = "";
        l.spans.push_back(s);
        diag::Diagnostic d;
        d.level = Level::Error;
        d.stage = Stage::Semantic;
        d.message = message;
        d.labels.push_back(l);
        return d;
    }

    static Diagnostic tokenizer_error(const std::string &message, const Location &loc) {
        diag::Span s;
        s.loc = loc;
        diag::Label l;
        l.primary = true;
        l.message = "";
        l.spans.push_back(s);
        diag::Diagnostic d;
        d.level = Level::Error;
        d.stage = Stage::Tokenizer;
        d.message = message;
        d.labels.push_back(l);
        return d;
    }

    static Diagnostic parser_error(const std::string &message, const Location &loc) {
        diag::Span s;
        s.loc = loc;
        diag::Label l;
        l.primary = true;
        l.message = "";
        l.spans.push_back(s);
        diag::Diagnostic d;
        d.level = Level::Error;
        d.stage = Stage::Parser;
        d.message = message;
        d.labels.push_back(l);
        return d;
    }

    static Diagnostic parser_error(const std::string &message) {
        diag::Diagnostic d;
        d.level = Level::Error;
        d.stage = Stage::Parser;
        d.message = message;
        return d;
    }

    static Diagnostic semantic_error_label(const std::string &message,
            const Location &loc, const std::string &error_label) {
        diag::Span s;
        s.loc = loc;
        diag::Label l;
        l.primary = true;
        l.message = error_label;
        l.spans.push_back(s);
        diag::Diagnostic d;
        d.level = Level::Error;
        d.stage = Stage::Semantic;
        d.message = message;
        d.labels.push_back(l);
        return d;
    }

    static Diagnostic tokenizer_error_label(const std::string &message,
            const Location &loc, const std::string &error_label) {
        diag::Span s;
        s.loc = loc;
        diag::Label l;
        l.primary = true;
        l.message = error_label;
        l.spans.push_back(s);
        diag::Diagnostic d;
        d.level = Level::Error;
        d.stage = Stage::Tokenizer;
        d.message = message;
        d.labels.push_back(l);
        return d;
    }

    static Diagnostic semantic_error_label(const std::string &message,
            const std::vector<Location> &locations, const std::string &error_label) {
        diag::Label l;
        l.primary = true;
        l.message = error_label;
        for (auto &loc : locations) {
            Span s;
            s.loc = loc;
            l.spans.push_back(s);

        }
        diag::Diagnostic d;
        d.level = Level::Error;
        d.stage = Stage::Semantic;
        d.message = message;
        d.labels.push_back(l);
        return d;
    }

    void secondary_label(const std::string &message,
            const Location &loc) {
        diag::Span s;
        s.loc = loc;
        diag::Label l;
        l.primary = false;
        l.message = message;
        l.spans.push_back(s);
        this->labels.push_back(l);
    }

    static Diagnostic codegen_error(const std::string &message) {
        diag::Diagnostic d;
        d.level = Level::Error;
        d.stage = Stage::CodeGen;
        d.message = message;
        return d;
    }

    static Diagnostic codegen_error(const std::string &message, const Location &loc) {
        diag::Span s;
        s.loc = loc;
        diag::Label l;
        l.primary = true;
        l.message = "";
        l.spans.push_back(s);
        diag::Diagnostic d;
        d.level = Level::Error;
        d.stage = Stage::CodeGen;
        d.message = message;
        d.labels.push_back(l);
        return d;
    }

/*
    private:
        Diagnostic() {}
*/
};

std::string render_diagnostic(const Diagnostic &d, bool use_colors);

} // namespace diag
} // namespace LFortran

#endif // LFORTRAN_DIAGNOSTICS_H
