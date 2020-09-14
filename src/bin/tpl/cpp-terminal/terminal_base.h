#ifndef TERMINAL_BASE_H
#define TERMINAL_BASE_H

/*
 * This file contains all the platform specific code regarding terminal input
 * and output. The rest of the code does not have any platform specific
 * details. This file is designed in a way to contain the least number of
 * building blocks, that the rest of the code can use to build all the
 * features.
 */

#include <stdexcept>

#ifdef _WIN32
#    include <conio.h>
#    define _WINSOCKAPI_
#    include <windows.h>
#    include <io.h>
#else
#    include <sys/ioctl.h>
#    include <termios.h>
#undef B0
#undef B50
#undef B75
#undef B110
#undef B134
#undef B150
#undef B200
#undef B300
#undef B600
#undef B1200
#undef B1800
#undef B2400
#undef B4800
#undef B9600
#undef B19200
#undef B28800
#undef B38400
#undef B57600
#undef B115200
#    include <unistd.h>
#    include <errno.h>
#endif

namespace Term {

/* Note: the code that uses Terminal must be inside try/catch block, otherwise
 * the destructors will not be called when an exception happens and the
 * terminal will not be left in a good state. Terminal uses exceptions when
 * something goes wrong.
 */
class BaseTerminal {
private:
#ifdef _WIN32
    HANDLE hout;
    DWORD dwOriginalOutMode;
    bool out_console;
    UINT out_code_page;

    HANDLE hin;
    DWORD dwOriginalInMode;
    UINT in_code_page;
#else
    struct termios orig_termios;
#endif
    bool keyboard_enabled;

public:
#ifdef _WIN32
    BaseTerminal(bool enable_keyboard=false, bool /*disable_ctrl_c*/ = true)
        : keyboard_enabled{enable_keyboard}
    {
        // Uncomment this to silently disable raw mode for non-tty
        //if (keyboard_enabled) keyboard_enabled = is_stdin_a_tty();
        out_console = is_stdout_a_tty();
        if (out_console) {
            hout = GetStdHandle(STD_OUTPUT_HANDLE);
            out_code_page = GetConsoleOutputCP();
            SetConsoleOutputCP(65001);
            if (hout == INVALID_HANDLE_VALUE) {
                throw std::runtime_error("GetStdHandle(STD_OUTPUT_HANDLE) failed");
            }
            if (!GetConsoleMode(hout, &dwOriginalOutMode)) {
                throw std::runtime_error("GetConsoleMode() failed");
            }
            DWORD flags = dwOriginalOutMode;
            flags |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            flags |= DISABLE_NEWLINE_AUTO_RETURN;
            if (!SetConsoleMode(hout, flags)) {
                throw std::runtime_error("SetConsoleMode() failed");
            }
        }

        if (keyboard_enabled) {
            hin = GetStdHandle(STD_INPUT_HANDLE);
            in_code_page = GetConsoleCP();
            SetConsoleCP(65001);
            if (hin == INVALID_HANDLE_VALUE) {
                throw std::runtime_error(
                        "GetStdHandle(STD_INPUT_HANDLE) failed");
            }
            if (!GetConsoleMode(hin, &dwOriginalInMode)) {
                throw std::runtime_error("GetConsoleMode() failed");
            }
            DWORD flags = dwOriginalInMode;
            flags |= ENABLE_VIRTUAL_TERMINAL_INPUT;
            flags &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
            if (!SetConsoleMode(hin, flags)) {
                throw std::runtime_error("SetConsoleMode() failed");
            }
        }
#else
    BaseTerminal(bool enable_keyboard=false, bool disable_ctrl_c=true)
        : keyboard_enabled{enable_keyboard}
    {
        // Uncomment this to silently disable raw mode for non-tty
        //if (keyboard_enabled) keyboard_enabled = is_stdin_a_tty();
        if (keyboard_enabled) {
            if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
                throw std::runtime_error("tcgetattr() failed");
            }

            // Put terminal in raw mode
            struct termios raw = orig_termios;
            raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
            // This disables output post-processing, requiring explicit \r\n. We
            // keep it enabled, so that in C++, one can still just use std::endl
            // for EOL instead of "\r\n".
            //raw.c_oflag &= ~(OPOST);
            raw.c_cflag |= (CS8);
            raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
            if (disable_ctrl_c) {
                raw.c_lflag &= ~(ISIG);
            }
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;

            if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
                throw std::runtime_error("tcsetattr() failed");
            }
        }
#endif
    }

    virtual ~BaseTerminal() noexcept(false)
    {
#ifdef _WIN32
        if (out_console) {
            SetConsoleOutputCP(out_code_page);
            if (!SetConsoleMode(hout, dwOriginalOutMode)) {
                throw std::runtime_error(
                        "SetConsoleMode() failed in destructor");
            }
        }

        if (keyboard_enabled) {
            SetConsoleCP(in_code_page);
            if (!SetConsoleMode(hin, dwOriginalInMode)) {
                throw std::runtime_error(
                        "SetConsoleMode() failed in destructor");
            }
        }
#else
        if (keyboard_enabled) {
            if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
                throw std::runtime_error("tcsetattr() failed in destructor");
            }
        }
#endif
    }

    // Returns true if a character is read, otherwise immediately returns false
    bool read_raw(char* s) const
    {
#ifdef _WIN32
        char buf[1];
        DWORD nread;
        if (_kbhit()) {
            if (!ReadFile(hin, buf, 1, &nread, nullptr)) {
                throw std::runtime_error("ReadFile() failed");
            }
            if (nread == 1) {
                *s = buf[0];
                return true;
            } else {
                throw std::runtime_error("kbhit() and ReadFile() inconsistent");
            }
        } else {
            return false;
        }
#else
        int nread = read(STDIN_FILENO, s, 1);
        if (nread == -1 && errno != EAGAIN) {
            throw std::runtime_error("read() failed");
        }
        return (nread == 1);
#endif
    }

    void get_term_size(int& rows, int& cols) const
    {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO inf;
        GetConsoleScreenBufferInfo(hout, &inf);
        cols = inf.srWindow.Right - inf.srWindow.Left + 1;
        rows = inf.srWindow.Bottom - inf.srWindow.Top + 1;
#else
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
            throw std::runtime_error(
                    "get_term_size() failed, probably not a terminal");
        } else {
            cols = ws.ws_col;
            rows = ws.ws_row;
        }
#endif
    }

    // Returns true if the standard input is attached to a terminal
    bool is_stdin_a_tty() const
    {
#ifdef _WIN32
        return _isatty(_fileno(stdin));
#else
        return isatty(STDIN_FILENO);
#endif
    }

    // Returns true if the standard output is attached to a terminal
    bool is_stdout_a_tty() const
    {
#ifdef _WIN32
        return _isatty(_fileno(stdout));
#else
        return isatty(STDOUT_FILENO);
#endif
    }
};

} // namespace Term

#endif // TERMINAL_BASE_H
