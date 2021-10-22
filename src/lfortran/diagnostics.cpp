#include <iomanip>
#include <sstream>

#include <lfortran/diagnostics.h>
#include <lfortran/assert.h>
#include <lfortran/exception.h>
#include <lfortran/codegen/fortran_evaluator.h>
#include <lfortran/parser/parser.h>

namespace LFortran::diag {

const static std::string redon  = "\033[0;31m";
const static std::string redoff = "\033[0;00m";

std::string highlight_line(const std::string &line,
        const size_t first_column,
        const size_t last_column,
        bool use_colors)
{
    if (first_column == 0 || last_column == 0) return "";
    if (last_column > line.size()+1) {
        throw LFortranException("The `last_column` in highlight_line is longer than the source line");
    }
    LFORTRAN_ASSERT(first_column >= 1)
    LFORTRAN_ASSERT(first_column <= last_column)
    LFORTRAN_ASSERT(last_column <= line.size()+1)
    std::stringstream out;
    if (line.size() > 0) {
        out << line.substr(0, first_column-1);
        if(use_colors) out << redon;
        if (last_column <= line.size()) {
            out << line.substr(first_column-1,
                    last_column-first_column+1);
        } else {
            // `last_column` points to the \n character
            out << line.substr(first_column-1,
                    last_column-first_column+1-1);
        }
        if(use_colors) out << redoff;
        if (last_column < line.size()) out << line.substr(last_column);
    }
    out << std::endl;
    if (first_column > 0) {
        for (size_t i=0; i < first_column-1; i++) {
            out << " ";
        }
    }
    if(use_colors) out << redon << "^";
    else out << "^";
    for (size_t i=first_column; i < last_column; i++) {
        out << "~";
    }
    if(use_colors) out << redoff;
    out << std::endl;
    return out.str();
}

bool Diagnostics::has_error() const {
    for (auto &d : this->diagnostics) {
        if (d.level == Level::Error) return true;
    }
    return false;
}

std::string Diagnostics::render(const std::string &input,
        const LocationManager &lm, const CompilerOptions &compiler_options) {
    std::string out;
    for (auto &d : this->diagnostics) {
        if (d.level == Level::Warning && compiler_options.no_warnings) {
            continue;
        }
        out += render_diagnostic(d, input, lm,
            compiler_options.use_colors,
            compiler_options.show_stacktrace);
        if (&d != &this->diagnostics.back()) out += "\n";
    }
    return out;
}

// Fills Diagnostic with span details and renders it
std::string render_diagnostic(Diagnostic &d, const std::string &input,
        const LocationManager &lm, bool use_colors, bool show_stacktrace) {
    std::string out;
    if (show_stacktrace) {
        out += FortranEvaluator::error_stacktrace(d.stacktrace);
    }
    // Convert to line numbers and get source code strings
    populate_spans(d, lm, input);
    // Render the message
    out += render_diagnostic(d, use_colors);
    return out;
}

std::string render_diagnostic(const Diagnostic &d, bool use_colors) {
    std::string bold  = "\033[0;1m";
    std::string red_bold  = "\033[0;31;1m";
    std::string yellow_bold  = "\033[0;33;1m";
    std::string green_bold  = "\033[0;32;1m";
    std::string blue_bold  = "\033[0;34;1m";
    std::string reset = "\033[0;00m";
    if (!use_colors) {
        bold = "";
        red_bold = "";
        yellow_bold = "";
        green_bold = "";
        blue_bold = "";
        reset = "";
    }
    std::stringstream out;

    std::string message_type = "";
    std::string primary_color = "";
    std::string type_color = "";
    switch (d.level) {
        case (Level::Error):
            primary_color = red_bold;    
            type_color = primary_color;
            switch (d.stage) {
                case (Stage::CPreprocessor):
                    message_type = "C preprocessor error";
                    break;
                case (Stage::Prescanner):
                    message_type = "prescanner error";
                    break;
                case (Stage::Tokenizer):
                    message_type = "tokenizer error";
                    break;
                case (Stage::Parser):
                    message_type = "syntax error";
                    break;
                case (Stage::Semantic):
                    message_type = "semantic error";
                    break;
                case (Stage::ASRPass):
                    message_type = "ASR pass error";
                    break;
                case (Stage::CodeGen):
                    message_type = "code generation error";
                    break;
            }
            break;
        case (Level::Warning):
            primary_color = yellow_bold;    
            type_color = primary_color;
            message_type = "warning";
            break;
        case (Level::Note):
            primary_color = bold;    
            type_color = primary_color;
            message_type = "note";
            break;
        case (Level::Help):
            primary_color = bold;    
            type_color = primary_color;
            message_type = "help";
            break;
        case (Level::Style):
            primary_color = green_bold;
            type_color = yellow_bold;
            message_type = "style suggestion";
            break;
    }

    out << type_color << message_type << reset << bold << ": " << d.message << reset << std::endl;

    if (d.labels.size() > 0) {
        Label l = d.labels[0];
        Span s = l.spans[0];
        int line_num_width = 1;
        if (s.last_line >= 10000) {
            line_num_width = 5;
        } else if (s.last_line >= 1000) {
            line_num_width = 4;
        } else if (s.last_line >= 100) {
            line_num_width = 3;
        } else if (s.last_line >= 10) {
            line_num_width = 2;
        }
        out << std::string(line_num_width, ' ') << blue_bold << "-->" << reset << " " << s.filename << ":" << s.first_line << ":" << s.first_column;
        if (s.first_line != s.last_line) {
            out << " - " << s.last_line << ":" << s.last_column;
        }
        out << std::endl;
        if (s.first_line == s.last_line) {
            // Primary label:
            out << std::string(line_num_width+1, ' ') << blue_bold << "|"
                << reset << std::endl;
            std::string line = s.source_code[0];
            out << blue_bold << std::setw(line_num_width)
                << std::to_string(s.first_line) << " |" << reset << " "
                << line << std::endl;
            out << std::string(line_num_width+1, ' ') << blue_bold << "|"
                << reset << " ";
            out << std::string(s.first_column-1, ' ');
            out << primary_color << std::string(s.last_column-s.first_column+1, '^');
            if (l.spans.size() == 2) {
                // For now we paint the second span only if it is "easy"
                Span s2=l.spans[1];
                if (s2.first_line == s2.last_line && s2.first_line == s.first_line)  {
                    if (s2.first_column > s.last_column+1) {
                        out << std::string(s2.first_column-s.last_column-1, ' ');
                        out << std::string(s2.last_column-s2.first_column+1, '^');
                    }
                }
            }
            out << " " << l.message << reset << std::endl;

            if (d.labels.size() == 2) {
                // Secondary label
                Label l = d.labels[1];
                Span s = l.spans[0];
                if (s.first_line == s.last_line) {
                    out << std::string(line_num_width+1, ' ') << blue_bold << "|"
                        << reset << std::endl;
                    std::string line = s.source_code[0];
                    out << blue_bold << std::setw(line_num_width)
                        << std::to_string(s.first_line) << " |" << reset << " "
                        << line << std::endl;
                    out << std::string(line_num_width+1, ' ') << blue_bold << "|"
                        << reset << " ";
                    out << std::string(s.first_column-1, ' ');
                    out << blue_bold << std::string(s.last_column-s.first_column+1, '~');
                    if (l.spans.size() == 2) {
                        // For now we paint the second span only if it is "easy"
                        Span s2=l.spans[1];
                        if (s2.first_line == s2.last_line && s2.first_line == s.first_line)  {
                            if (s2.first_column > s.last_column+1) {
                                out << std::string(s2.first_column-s.last_column-1, ' ');
                                out << std::string(s2.last_column-s2.first_column+1, '~');
                            }
                        }
                    }
                    out << " " << l.message << reset << std::endl;
                }
            }
        } else {
            out << "first (" << s.first_line << ":" << s.first_column;
            out << ")" << std::endl;
            std::string line = s.source_code[0];
            if (s.first_column <= line.size()) {
                out << highlight_line(line, s.first_column, line.size(), use_colors);
            }
            out << "last (" << s.last_line << ":" << s.last_column;
            out << ")" << std::endl;
            line = s.source_code[s.source_code.size()-1];
            out << highlight_line(line, 1, s.last_column, use_colors);
        }
    }
    return out.str();
}

} // namespace LFortran::diag
