#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#    include <unistd.h>
#endif

#include <xeus/xinterpreter.hpp>
#include <xeus/xkernel.hpp>
#include <xeus/xkernel_configuration.hpp>
#include <nlohmann/json.hpp>

#include <lfortran/fortran_kernel.h>
#include <lfortran/parser/parser.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/codegen/asr_to_llvm.h>
#include <lfortran/codegen/evaluator.h>
#include <lfortran/asr_utils.h>

namespace nl = nlohmann;

namespace LFortran
{

    bool startswith(const std::string &s, const std::string &e) {
        if (s.size() < e.size()) return false;
        return s.substr(0, e.size()) == e;
    }

    class RedirectStdout
    {
    public:
        RedirectStdout(std::string &out) : _out{out} {
#ifndef _WIN32
            std::cout << std::flush;
            fflush(stdout);
            saved_stdout = dup(STDOUT_FILENO);
            if(pipe(out_pipe) != 0) {
                throw LFortranException("pipe() failed");
            }
            dup2(out_pipe[1], STDOUT_FILENO);
            close(out_pipe[1]);
            printf("X");
#endif
        }

        ~RedirectStdout() {
#ifndef _WIN32
            fflush(stdout);
            read(out_pipe[0], buffer, MAX_LEN);
            dup2(saved_stdout, STDOUT_FILENO);
            _out = std::string(&buffer[1]);
#endif
        }
    private:
        std::string &_out;
        static const size_t MAX_LEN=1024;
        char buffer[MAX_LEN+1] = {0};
        int out_pipe[2];
        int saved_stdout;
    };

    class custom_interpreter : public xeus::xinterpreter
    {
    private:
        FortranEvaluator e;

    public:
        custom_interpreter() = default;
        virtual ~custom_interpreter() = default;

    private:

        void configure_impl() override;

        nl::json execute_request_impl(int execution_counter,
                                      const std::string& code,
                                      bool silent,
                                      bool store_history,
                                      nl::json user_expressions,
                                      bool allow_stdin) override;

        nl::json complete_request_impl(const std::string& code,
                                       int cursor_pos) override;

        nl::json inspect_request_impl(const std::string& code,
                                      int cursor_pos,
                                      int detail_level) override;

        nl::json is_complete_request_impl(const std::string& code) override;

        nl::json kernel_info_request_impl() override;

        void shutdown_request_impl() override;
    };


    nl::json custom_interpreter::execute_request_impl(int execution_counter, // Typically the cell number
                                                      const std::string& code, // Code to execute
                                                      bool /*silent*/,
                                                      bool /*store_history*/,
                                                      nl::json /*user_expressions*/,
                                                      bool /*allow_stdin*/)
    {
        FortranEvaluator::EvalResult r;
        std::string std_out;
        std::string code0;
        try {
            if (startswith(code, "%%showast")) {
                FortranEvaluator::Result<std::string> res;
                code0 = code.substr(code.find("\n")+1);
                res = e.get_ast(code0);
                nl::json result;
                if (res.ok) {
                    publish_stream("stdout", res.result);
                    result["status"] = "ok";
                    result["payload"] = nl::json::array();
                    result["user_expressions"] = nl::json::object();
                } else {
                    std::string msg = e.format_error(res.error, code0);
                    publish_stream("stderr", msg);
                    result["status"] = "error";
                    result["ename"] = "CompilerError";
                    result["evalue"] = msg;
                    result["traceback"] = {};
                }
                return result;
            }
            if (startswith(code, "%%showasr")) {
                FortranEvaluator::Result<std::string> res;
                code0 = code.substr(code.find("\n")+1);
                res = e.get_asr(code0);
                nl::json result;
                if (res.ok) {
                    publish_stream("stdout", res.result);
                    result["status"] = "ok";
                    result["payload"] = nl::json::array();
                    result["user_expressions"] = nl::json::object();
                } else {
                    std::string msg = e.format_error(res.error, code0);
                    publish_stream("stderr", msg);
                    result["status"] = "error";
                    result["ename"] = "CompilerError";
                    result["evalue"] = msg;
                    result["traceback"] = {};
                }
                return result;
            }
            if (startswith(code, "%%showllvm")) {
                FortranEvaluator::Result<std::string> res;
                code0 = code.substr(code.find("\n")+1);
                res = e.get_llvm(code0);
                nl::json result;
                if (res.ok) {
                    publish_stream("stdout", res.result);
                    result["status"] = "ok";
                    result["payload"] = nl::json::array();
                    result["user_expressions"] = nl::json::object();
                } else {
                    std::string msg = e.format_error(res.error, code0);
                    publish_stream("stderr", msg);
                    result["status"] = "error";
                    result["ename"] = "CompilerError";
                    result["evalue"] = msg;
                    result["traceback"] = {};
                }
                return result;
            }
            if (startswith(code, "%%showasm")) {
                FortranEvaluator::Result<std::string> res;
                code0 = code.substr(code.find("\n")+1);
                res = e.get_asm(code0);
                nl::json result;
                if (res.ok) {
                    publish_stream("stdout", res.result);
                    result["status"] = "ok";
                    result["payload"] = nl::json::array();
                    result["user_expressions"] = nl::json::object();
                } else {
                    std::string msg = e.format_error(res.error, code0);
                    publish_stream("stderr", msg);
                    result["status"] = "error";
                    result["ename"] = "CompilerError";
                    result["evalue"] = msg;
                    result["traceback"] = {};
                }
                return result;
            }
            if (startswith(code, "%%showcpp")) {
                std::string s;
                code0 = code.substr(code.find("\n")+1);
                s = e.get_cpp(code0);
                publish_stream("stdout", s);
                nl::json result;
                result["status"] = "ok";
                result["payload"] = nl::json::array();
                result["user_expressions"] = nl::json::object();
                return result;
            }
            if (startswith(code, "%%showfmt")) {
                std::string s;
                code0 = code.substr(code.find("\n")+1);
                s = e.get_fmt(code0);
                publish_stream("stdout", s);
                nl::json result;
                result["status"] = "ok";
                result["payload"] = nl::json::array();
                result["user_expressions"] = nl::json::object();
                return result;
            }

            RedirectStdout s(std_out);
            code0 = code;
            FortranEvaluator::Result<FortranEvaluator::EvalResult> res;
            res = e.evaluate(code0);
            if (res.ok) {
                r = res.result;
            } else {
                std::string msg = e.format_error(res.error, code0);
                publish_stream("stderr", msg);
                nl::json result;
                result["status"] = "error";
                result["ename"] = "CompilerError";
                result["evalue"] = msg;
                result["traceback"] = {};
                return result;
            }
        } catch (const LFortranException &e) {
            publish_stream("stderr", "LFortran Exception: " + e.msg());
            nl::json result;
            result["status"] = "error";
            result["ename"] = "LFortranException";
            result["evalue"] = e.msg();
            result["traceback"] = {};
            return result;
        }

        if (std_out.size() > 0) {
            publish_stream("stdout", std_out);
        }

        switch (r.type) {
            case (LFortran::FortranEvaluator::EvalResult::integer) : {
                nl::json pub_data;
                pub_data["text/plain"] = std::to_string(r.i);
                publish_execution_result(execution_counter, std::move(pub_data), nl::json::object());
                break;
            }
            case (LFortran::FortranEvaluator::EvalResult::real) : {
                nl::json pub_data;
                pub_data["text/plain"] = std::to_string(r.f);
                publish_execution_result(execution_counter, std::move(pub_data), nl::json::object());
                break;
            }
            case (LFortran::FortranEvaluator::EvalResult::statement) : {
                break;
            }
            case (LFortran::FortranEvaluator::EvalResult::none) : {
                break;
            }
            default : throw LFortranException("Return type not supported");
        }

        nl::json result;
        result["status"] = "ok";
        result["payload"] = nl::json::array();
        result["user_expressions"] = nl::json::object();
        return result;
    }

    void custom_interpreter::configure_impl()
    {
        // Perform some operations
    }

    nl::json custom_interpreter::complete_request_impl(const std::string& code,
                                                       int cursor_pos)
    {
        nl::json result;

        // Code starts with 'H', it could be the following completion
        if (code[0] == 'H')
        {
            result["status"] = "ok";
            result["matches"] = {"Hello", "Hey", "Howdy"};
            result["cursor_start"] = 5;
            result["cursor_end"] = cursor_pos;
        }
        // No completion result
        else
        {
            result["status"] = "ok";
            result["matches"] = nl::json::array();
            result["cursor_start"] = cursor_pos;
            result["cursor_end"] = cursor_pos;
        }

        return result;
    }

    nl::json custom_interpreter::inspect_request_impl(const std::string& code,
                                                      int /*cursor_pos*/,
                                                      int /*detail_level*/)
    {
        nl::json result;

        if (code.compare("print") == 0)
        {
            result["found"] = true;
            result["text/plain"] = "Print objects to the text stream file, [...]";
        }
        else
        {
            result["found"] = false;
        }

        result["status"] = "ok";
        return result;
    }

    nl::json custom_interpreter::is_complete_request_impl(const std::string& /*code*/)
    {
        nl::json result;

        // if (is_complete(code))
        // {
            result["status"] = "complete";
        // }
        // else
        // {
        //    result["status"] = "incomplete";
        //    result["indent"] = 4;
        //}

        return result;
    }

    nl::json custom_interpreter::kernel_info_request_impl()
    {
        nl::json result;
        std::string version = LFORTRAN_VERSION;
        std::string banner = ""
            "LFortran " + version + "\n"
            "Jupyter kernel for Fortran";
        result["banner"] = banner;
        result["implementation"] = "LFortran";
        result["implementation_version"] = version;
        result["language_info"]["name"] = "fortran";
        result["language_info"]["version"] = "2018";
        result["language_info"]["mimetype"] = "text/x-fortran";
        result["language_info"]["file_extension"] = ".f90";
        return result;
    }

    void custom_interpreter::shutdown_request_impl() {
        std::cout << "Bye!!" << std::endl;
    }

    int run_kernel(const std::string &connection_filename)
    {
        // Load configuration file
        xeus::xconfiguration config = xeus::load_configuration(connection_filename);

        // Create interpreter instance
        using interpreter_ptr = std::unique_ptr<custom_interpreter>;
        interpreter_ptr interpreter = interpreter_ptr(new custom_interpreter());

        // Create kernel instance and start it
        xeus::xkernel kernel(config, xeus::get_user_name(), std::move(interpreter));
        kernel.start();

        return 0;
    }

}
