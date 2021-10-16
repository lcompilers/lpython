#include <sstream>

#include <lfortran/diagnostics.h>

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


std::string render_diagnostic(const Diagnostic &d, bool use_colors) {
    std::stringstream out;
    Label l = d.labels[0];
    Span s = l.spans[0];
    out << s.filename << ":" << s.first_line << ":" << s.first_column;
    if (s.first_line != s.last_line) {
        out << " - " << s.last_line << ":" << s.last_column;
    }
    if (use_colors) out << " " << redon << "semantic error:" << redoff << " ";
    else out << " " << "semantic error:" <<  " ";
    out << d.message << std::endl;
    if (s.first_line == s.last_line) {
        std::string line = s.source_code[0];
        out << highlight_line(line, s.first_column, s.last_column, use_colors);
    } else {
        out << "first (" << s.first_line << ":" << s.first_column;
        out << ")" << std::endl;
        std::string line = s.source_code[0];
        out << highlight_line(line, s.first_column, line.size(), use_colors);
        out << "last (" << s.last_line << ":" << s.last_column;
        out << ")" << std::endl;
        line = s.source_code[s.source_code.size()-1];
        out << highlight_line(line, 1, s.last_column, use_colors);
    }
    return out.str();
}

} // namespace LFortran::diag
