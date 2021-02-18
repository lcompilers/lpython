#include "terminal.h"

#include <vector>

using Term::Terminal;
using Term::Key;
using Term::move_cursor;
using Term::cursor_off;
using Term::cursor_on;

// This model contains all the information about the state of the prompt in an
// abstract way, irrespective of where or how it is rendered.
struct Model {
    std::string prompt_string; // The string to show as the prompt
    std::vector<std::string> lines; // The current input string in the prompt as
            // a vector of lines, without '\n' at the end.
    // The current cursor position in the "input" string, starting from (1,1)
    size_t cursor_col, cursor_row;
};

std::string concat(const std::vector<std::string> &lines) {
    std::string s;
    for (auto &line: lines) {
        s.append(line + "\n");
    }
    return s;
}

std::vector<std::string> split(const std::string &s) {
    size_t j = 0;
    std::vector<std::string> lines;
    lines.push_back("");
    if (s[s.size()-1] != '\n') throw std::runtime_error("\\n is required");
    for (size_t i=0; i<s.size()-1; i++) {
        if (s[i] == '\n') {
            j++;
            lines.push_back("");
        } else {
            lines[j].push_back(s[i]);
        }
    }
    return lines;
}

char32_t U(const std::string &s) {
    std::u32string s2 = Term::utf8_to_utf32(s);
    if (s2.size() != 1) throw std::runtime_error("U(s): s not a codepoint.");
    return s2[0];
}

void print_left_curly_bracket(Term::Window &scr, int x, int y1, int y2) {
    int h = y2-y1+1;
    if (h == 1) {
        scr.set_char(x, y1, U("]"));
    } else {
        scr.set_char(x, y1, U("┐"));
        for (int j=y1+1; j <= y2-1; j++) {
            scr.set_char(x, j, U("│"));
        }
        scr.set_char(x, y2, U("┘"));
    }
}

void render(Term::Window &scr, const Model &m, size_t cols) {
    scr.clear();
    print_left_curly_bracket(scr, cols, 1, m.lines.size());
    scr.print_str(cols-6, m.lines.size(), std::to_string(m.cursor_row) + ","
            + std::to_string(m.cursor_col));
    for (size_t j=0; j < m.lines.size(); j++) {
        if (j == 0) {
            scr.print_str(1, j+1, m.prompt_string);
        } else {
            for (size_t i=0; i < m.prompt_string.size()-1; i++) {
                scr.set_char(i+1, j+1, '.');
            }
        }
        scr.print_str(m.prompt_string.size()+1, j+1, m.lines[j]);
    }
    scr.set_cursor_pos(m.prompt_string.size() + m.cursor_col, m.cursor_row);
}

std::string prompt0(const Terminal &term, const std::string &prompt_string,
        std::vector<std::string> &history) {
    int row, col;
    term.get_cursor_position(row, col);
    int rows, cols;
    term.get_term_size(rows, cols);

    Model m;
    m.prompt_string = prompt_string;
    m.lines.push_back("");
    m.cursor_col = 1;
    m.cursor_row = 1;

    // Make a local copy of history that can be modified by the user. All
    // changes will be forgotten once a command is submitted.
    std::vector<std::string> hist = history;
    size_t history_pos = hist.size();
    hist.push_back(concat(m.lines)); // Push back empty input

    Term::Window scr(cols, 1);
    int key;
    render(scr, m, cols);
    std::cout << scr.render(1, row) << std::flush;
    while ((key = term.read_key()) != Key::ENTER) {
        if (  (key >= 'a' && key <= 'z') ||
              (key >= 'A' && key <= 'Z') ||
              (!iscntrl(key) && key < 128)  ) {
            std::string before = m.lines[m.cursor_row-1].substr(0,
                    m.cursor_col-1);
            std::string newchar; newchar.push_back(key);
            std::string after = m.lines[m.cursor_row-1].substr(m.cursor_col-1);
            m.lines[m.cursor_row-1] = before + newchar + after;
            m.cursor_col++;
        } else if (key == CTRL_KEY('d')) {
            if (m.lines.size() == 1 && m.lines[m.cursor_row-1].size() == 0) {
                m.lines[m.cursor_row-1].push_back(CTRL_KEY('d'));
                std::cout << "\n" << std::flush;
                history.push_back(m.lines[0]);
                return m.lines[0];
            }
        } else {
            switch (key) {
                case Key::BACKSPACE:
                    if (m.cursor_col > 1) {
                        std::string before = m.lines[m.cursor_row-1].substr(0,
                                m.cursor_col-2);
                        std::string after = m.lines[m.cursor_row-1]
                                .substr(m.cursor_col-1);
                        m.lines[m.cursor_row-1] = before + after;
                        m.cursor_col--;
                    } else if (m.cursor_col == 1 && m.cursor_row > 1) {
                        m.cursor_col = m.lines[m.cursor_row-2].size() + 1;
                        m.lines[m.cursor_row-2] += m.lines[m.cursor_row-1];
                        m.lines.erase(m.lines.begin() + m.cursor_row-1);
                        m.cursor_row--;
                        rows--;
                    }
                    break;
                case Key::ARROW_LEFT:
                    if (m.cursor_col > 1) {
                        m.cursor_col--;
                    }
                    break;
                case Key::ARROW_RIGHT:
                    if (m.cursor_col <= m.lines[m.cursor_row-1].size()) {
                        m.cursor_col++;
                    }
                    break;
                case Key::HOME:
                    m.cursor_col = 1;
                    break;
                case Key::END:
                    m.cursor_col = m.lines[m.cursor_row-1].size()+1;
                    break;
                case Key::ARROW_UP:
                    if (m.cursor_row == 1) {
                        if (history_pos > 0) {
                            hist[history_pos] = concat(m.lines);
                            history_pos--;
                            m.lines = split(hist[history_pos]);
                            m.cursor_row = m.lines.size();
                            if (m.cursor_col>m.lines[m.cursor_row-1].size()+1) {
                                m.cursor_col = m.lines[m.cursor_row-1].size()+1;
                            }
                            if (m.lines.size() > scr.get_h()) {
                                scr.set_h(m.lines.size());
                            }
                        }
                    } else {
                        m.cursor_row--;
                        if (m.cursor_col > m.lines[m.cursor_row-1].size()+1) {
                            m.cursor_col = m.lines[m.cursor_row-1].size()+1;
                        }
                    }
                    break;
                case Key::ARROW_DOWN:
                    if (m.cursor_row == m.lines.size()) {
                        if (history_pos < hist.size()-1) {
                            hist[history_pos] = concat(m.lines);
                            history_pos++;
                            m.lines = split(hist[history_pos]);
                            m.cursor_row = 1;
                            if (m.cursor_col>m.lines[m.cursor_row-1].size()+1) {
                                m.cursor_col = m.lines[m.cursor_row-1].size()+1;
                            }
                            if (m.lines.size() > scr.get_h()) {
                                scr.set_h(m.lines.size());
                            }
                        }
                    } else {
                        m.cursor_row++;
                        if (m.cursor_col > m.lines[m.cursor_row-1].size()+1) {
                            m.cursor_col = m.lines[m.cursor_row-1].size()+1;
                        }
                    }
                    break;
                case ALT_KEY('n'):
                case Key::ALT_ENTER:
                    std::string before = m.lines[m.cursor_row-1].substr(0,
                            m.cursor_col-1);
                    std::string after = m.lines[m.cursor_row-1]
                            .substr(m.cursor_col-1);
                    m.lines[m.cursor_row-1] = before;
                    m.lines.push_back(after);
                    m.cursor_col = after.size()+1;
                    m.cursor_row++;
                    scr.set_h(scr.get_h()+1);
            }
        }
        render(scr, m, cols);
        std::cout << scr.render(1, row) << std::flush;
        if (row+(int)scr.get_h()-1 > rows) {
            row = rows - ((int)scr.get_h()-1);
        }
    }
    std::cout << "\n" << std::flush;
    history.push_back(concat(m.lines));
    return concat(m.lines);
}
