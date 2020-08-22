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
        Allocator al;
        LLVMEvaluator e;
        SymbolTable *symbol_table;

    public:
        custom_interpreter() : al{Allocator(64*1024*1024)},
            symbol_table{nullptr} { }
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
        if (code == "1x") {
            // publish_execution_error(error_name, error_value, error_traceback);
            if (!silent) {
                publish_execution_error("ParseError", "1x cannot be parsed", {"a", "b"});
            }

            nl::json result;
            result["status"] = "error";
            result["ename"] = "ParseError";
            result["evalue"] = "1x cannot be parsed";
            result["traceback"] = {"a", "b"};
            return result;
        }

        if (code == "print *, \"hello, world\"") {
            publish_stream("stdout", "hello, world\n");
            nl::json result;
            result["status"] = "ok";
            result["payload"] = nl::json::array();
            result["user_expressions"] = nl::json::object();
            return result;
        }

        if (code == "error stop") {
            publish_stream("stderr", "ERROR STOP\n");
            nl::json result;
            result["status"] = "ok";
            result["payload"] = nl::json::array();
            result["user_expressions"] = nl::json::object();
            return result;
        }

        // Src -> AST
        AST::TranslationUnit_t* ast;
        try {
            ast = parse2(al, code);
        } catch (const TokenizerError &e) {
            nl::json result;
            result["status"] = "error";
            result["ename"] = "TokenizerError";
            result["evalue"] = e.msg();
            result["traceback"] = {};
            return result;
        } catch (const ParserError &e) {
            nl::json result;
            result["status"] = "error";
            result["ename"] = "ParserError";
            result["evalue"] = e.msg();
            result["traceback"] = {};
            return result;
        } catch (const LFortranException &e) {
            nl::json result;
            result["status"] = "error";
            result["ename"] = "LFortranException";
            result["evalue"] = e.msg();
            result["traceback"] = {};
            return result;
        }


        // AST -> ASR
        ASR::TranslationUnit_t* asr;
        // Remove the old executation function if it exists
        if (symbol_table) {
            if (symbol_table->scope.find("f") != symbol_table->scope.end()) {
                symbol_table->scope.erase("f");
            }
            symbol_table->mark_all_variables_external();
        }
        try {
            asr = ast_to_asr(al, *ast, symbol_table);
        } catch (const LFortranException &e) {
            nl::json result;
            result["status"] = "error";
            result["ename"] = "LFortranException";
            result["evalue"] = e.msg();
            result["traceback"] = {};
            return result;
        }
        if (!symbol_table) symbol_table = asr->m_global_scope;

        // ASR -> LLVM
        std::unique_ptr<LLVMModule> m;
        try {
            m = asr_to_llvm(*asr, e.get_context(), al);
        } catch (const CodeGenError &e) {
            nl::json result;
            result["status"] = "error";
            result["ename"] = "CodeGenException";
            result["evalue"] = e.msg();
            result["traceback"] = {};
            return result;
        }

        std::string return_type = m->get_return_type("f");

        // LLVM -> Machine code -> Execution
        e.add_module(std::move(m));
        if (return_type == "integer") {
            int r = e.intfn("f");
            nl::json pub_data;
            pub_data["text/plain"] = std::to_string(r);
            publish_execution_result(execution_counter, std::move(pub_data), nl::json::object());
        } else if (return_type == "real") {
            float r = e.floatfn("f");
            std::cout << r << std::endl;
            nl::json pub_data;
            pub_data["text/plain"] = std::to_string(r);
            publish_execution_result(execution_counter, std::move(pub_data), nl::json::object());
        } else if (return_type == "void") {
            e.voidfn("f");
        } else if (return_type == "none") {
        } else {
            throw LFortranException("Return type not supported");
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
        std::string banner = ""
            "LFortran\n"
            "Jupyter kernel for Fortran\n"
            "Fortran";
        result["banner"] = banner;
        result["implementation"] = "my_kernel";
        result["implementation_version"] = "0.1.0";
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
