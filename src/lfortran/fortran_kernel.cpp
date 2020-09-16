#include <iostream>

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
                                                      bool silent,
                                                      bool /*store_history*/,
                                                      nl::json /*user_expressions*/,
                                                      bool /*allow_stdin*/)
    {
        if (code == "print *, \"hello, world\"") {
            publish_stream("stdout", "hello, world\n");
            nl::json result;
            result["status"] = "ok";
            result["payload"] = nl::json::array();
            result["user_expressions"] = nl::json::object();
            return result;
        }

        FortranEvaluator::Result r;
        try {
            r = e.evaluate(code);
        } catch (const TokenizerError &e) {
            std::string error;
            error = format_syntax_error("input", code, e.loc, -1, &e.token);
            publish_stream("stderr", error);
            nl::json result;
            result["status"] = "error";
            result["ename"] = "TokenizerError";
            result["evalue"] = e.msg();
            result["traceback"] = {};
            return result;
        } catch (const ParserError &e) {
            int token;
            if (e.msg() == "syntax is ambiguous") {
                token = -2;
            } else {
                token = e.token;
            }
            std::string error;
            error = format_syntax_error("input", code, e.loc, token);
            publish_stream("stderr", error);
            nl::json result;
            result["status"] = "error";
            result["ename"] = "ParserError";
            result["evalue"] = e.msg();
            result["traceback"] = {};
            return result;
        } catch (const SemanticError &e) {
            std::string error;
            error = format_semantic_error("input", code, e.loc, e.msg());
            publish_stream("stderr", error);
            nl::json result;
            result["status"] = "error";
            result["ename"] = "SemanticError";
            result["evalue"] = e.msg();
            result["traceback"] = {};
            return result;
        } catch (const CodeGenError &e) {
            publish_stream("stderr", "Code Generation Error: " + e.msg());
            nl::json result;
            result["status"] = "error";
            result["ename"] = "CodeGenException";
            result["evalue"] = e.msg();
            result["traceback"] = {};
            return result;
        } catch (const LFortranException &e) {
            publish_stream("stderr", "LFortran Exception: " + e.msg());
            nl::json result;
            result["status"] = "error";
            result["ename"] = "LFortranException";
            result["evalue"] = e.msg();
            result["traceback"] = {};
            return result;
        }

        switch (r.type) {
            case (LFortran::FortranEvaluator::ResultType::integer) : {
                nl::json pub_data;
                pub_data["text/plain"] = std::to_string(r.i);
                publish_execution_result(execution_counter, std::move(pub_data), nl::json::object());
                break;
            }
            case (LFortran::FortranEvaluator::ResultType::real) : {
                nl::json pub_data;
                pub_data["text/plain"] = std::to_string(r.f);
                publish_execution_result(execution_counter, std::move(pub_data), nl::json::object());
                break;
            }
            case (LFortran::FortranEvaluator::ResultType::statement) : {
                break;
            }
            case (LFortran::FortranEvaluator::ResultType::none) : {
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
