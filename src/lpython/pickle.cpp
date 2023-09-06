#include <string>

#include <lpython/pickle.h>
#include <lpython/bigint.h>
#include <lpython/python_ast.h>
#include <libasr/asr_utils.h>
#include <libasr/string_utils.h>
#include <libasr/location.h>
#include <libasr/pass/intrinsic_function_registry.h>
#include <libasr/pass/intrinsic_array_function_registry.h>

namespace LCompilers::LPython {

/********************** AST Pickle *******************/
class PickleVisitor : public AST::PickleBaseVisitor<PickleVisitor>
{
public:
    std::string get_str() {
        return s;
    }
};

std::string pickle_python(AST::ast_t &ast, bool colors, bool indent) {
    PickleVisitor v;
    v.use_colors = colors;
    v.indent = indent;
    v.visit_ast(ast);
    return v.get_str();
}

/********************** ASR Pickle *******************/
class ASRPickleVisitor :
    public ASR::PickleBaseVisitor<ASRPickleVisitor>
{
public:
    bool show_intrinsic_modules;

    std::string get_str() {
        return s;
    }
    void visit_symbol(const ASR::symbol_t &x) {
        s.append(ASRUtils::symbol_parent_symtab(&x)->get_counter());
        s.append(" ");
        if (use_colors) {
            s.append(color(fg::yellow));
        }
        s.append(ASRUtils::symbol_name(&x));
        if (use_colors) {
            s.append(color(fg::reset));
        }
    }
    void visit_IntegerConstant(const ASR::IntegerConstant_t &x) {
        s.append("(");
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("IntegerConstant");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append(" ");
        if (use_colors) {
            s.append(color(fg::cyan));
        }
        s.append(std::to_string(x.m_n));
        if (use_colors) {
            s.append(color(fg::reset));
        }
        s.append(" ");
        this->visit_ttype(*x.m_type);
        s.append(")");
    }
    void visit_Module(const ASR::Module_t &x) {
        // hide intrinsic modules and numpy module by default
        if (!show_intrinsic_modules &&
            (x.m_intrinsic || startswith(x.m_name, "numpy"))) {
            s.append("(");
            if (use_colors) {
                s.append(color(style::bold));
                s.append(color(fg::magenta));
            }
            s.append(x.m_intrinsic ? "IntrinsicModule" : "Module");
            if (use_colors) {
                s.append(color(fg::reset));
                s.append(color(style::reset));
            }
            s.append(" ");
            s.append(x.m_name);
            s.append(")");
        } else {
            ASR::PickleBaseVisitor<ASRPickleVisitor>::visit_Module(x);
        };
    }

    std::string convert_intrinsic_id(int x) {
        std::string s;
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::green));
        }
        s.append(ASRUtils::get_intrinsic_name(x));
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        return s;
    }

    std::string convert_impure_intrinsic_id(int x) {
        std::string s;
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::green));
        }
        s.append(ASRUtils::get_impure_intrinsic_name(x));
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        return s;
    }

    std::string convert_array_intrinsic_id(int x) {
        std::string s;
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::green));
        }
        s.append(ASRUtils::get_array_intrinsic_name(x));
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        return s;
    }
};

std::string pickle(ASR::asr_t &asr, bool colors, bool indent,
        bool show_intrinsic_modules) {
    ASRPickleVisitor v;
    v.use_colors = colors;
    v.indent = indent;
    v.show_intrinsic_modules = show_intrinsic_modules;
    v.visit_asr(asr);
    return v.get_str();
}

std::string pickle(ASR::TranslationUnit_t &asr, bool colors, bool indent, bool show_intrinsic_modules) {
    return pickle((ASR::asr_t &)asr, colors, indent, show_intrinsic_modules);
}

/********************** AST Pickle Tree *******************/
class ASTTreeVisitor : public AST::TreeBaseVisitor<ASTTreeVisitor>
{
public:
    std::string get_str() {
        return s;
    }
    void visit_Module(const AST::Module_t &x) {
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("Module");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append("\n├─body=↧");
        for (size_t i=0; i<x.n_body; i++) {
            inc_lindent();
            last = i == x.n_body-1;
            attached = false;
            this->visit_stmt(*x.m_body[i]);
            dec_indent();
        }
        s.append("\n╰─type_ignores=↧");
        for (size_t i=0; i<x.n_type_ignores; i++) {
            inc_indent();
            last = i == x.n_type_ignores-1;
            attached = false;
            this->visit_type_ignore(*x.m_type_ignores[i]);
            dec_indent();
        }
    }
};

std::string pickle_tree_python(AST::ast_t &ast, bool colors) {
    ASTTreeVisitor v;
    v.use_colors = colors;
    v.visit_ast(ast);
    return v.get_str();
}

/********************** ASR Pickle Tree *******************/
class ASRTreeVisitor :
    public ASR::TreeBaseVisitor<ASRTreeVisitor>
{
public:
    bool show_intrinsic_modules;

    std::string get_str() {
        return s;
    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        if(!attached) {
            attached = true;
        }
        if (use_colors) {
            s.append(color(style::bold));
            s.append(color(fg::magenta));
        }
        s.append("TranslationUnit");
        if (use_colors) {
            s.append(color(fg::reset));
            s.append(color(style::reset));
        }
        s.append("\n├─");
        inc_lindent();
        if (use_colors) {
            s.append(color(fg::yellow));
        }
        s.append("SymbolTable");
        if (use_colors) {
            s.append(color(fg::reset));
        }
        s.append("\n" + indtd + "├─counter=");
        s.append(x.m_global_scope->get_counter());
        size_t i = 0;
        s.append("\n" + indtd + "╰─scope=↧");
        for (auto &a : x.m_global_scope->get_scope()) {
            i++;
            inc_indent();
            last = i == x.m_global_scope->get_scope().size();
            s.append("\n" + indtd + (last ? "╰─" : "├─") + a.first + ": ");
            this->visit_symbol(*a.second);
            dec_indent();
        }
        dec_indent();
        s.append("\n╰─items=↧");
        attached = false;
        for (size_t i=0; i<x.n_items; i++) {
            inc_indent();
            last = i == x.n_items-1;
            attached = false;
            this->visit_asr(*x.m_items[i]);
            dec_indent();
        }
    }

};

std::string pickle_tree(ASR::asr_t &asr, bool colors, bool show_intrinsic_modules) {
    ASRTreeVisitor v;
    v.use_colors = colors;
    v.show_intrinsic_modules = show_intrinsic_modules;
    v.visit_asr(asr);
    return v.get_str();
}

std::string pickle_tree(ASR::TranslationUnit_t &asr, bool colors, bool show_intrinsic_modules) {
    return pickle_tree((ASR::asr_t &)asr, colors, show_intrinsic_modules);
}

/********************** AST Pickle Json *******************/
class ASTJsonVisitor :
    public LPython::AST::JsonBaseVisitor<ASTJsonVisitor>
{
public:
    using LPython::AST::JsonBaseVisitor<ASTJsonVisitor>::JsonBaseVisitor;

    std::string get_str() {
        return s;
    }
};

std::string pickle_json(LPython::AST::ast_t &ast, LocationManager &lm) {
    ASTJsonVisitor v(lm);
    v.visit_ast(ast);
    return v.get_str();
}

/********************** ASR Pickle Json *******************/
class ASRJsonVisitor :
    public ASR::JsonBaseVisitor<ASRJsonVisitor>
{
public:
    bool show_intrinsic_modules;

    using ASR::JsonBaseVisitor<ASRJsonVisitor>::JsonBaseVisitor;

    std::string get_str() {
        return s;
    }

    void visit_symbol(const ASR::symbol_t &x) {
        s.append("\"");
        s.append(ASRUtils::symbol_name(&x));
        s.append(" (SymbolTable");
        s.append(ASRUtils::symbol_parent_symtab(&x)->get_counter());
        s.append(")\"");
    }

    void visit_Module(const ASR::Module_t &x) {
        // hide intrinsic modules and numpy module by default
        if (!show_intrinsic_modules &&
            (x.m_intrinsic || startswith(x.m_name, "numpy"))) {
            s.append("{");
            inc_indent(); s.append("\n" + indtd);
            s.append("\"node\": \"Module\"");
            s.append(",\n" + indtd);
            s.append("\"fields\": {");
            inc_indent(); s.append("\n" + indtd);
            s.append("\"name\": ");
            s.append("\"" + std::string(x.m_name) + "\"");
            s.append(",\n" + indtd);
            s.append("\"dependencies\": ");
            s.append("[");
            if (x.n_dependencies > 0) {
                inc_indent(); s.append("\n" + indtd);
                for (size_t i=0; i<x.n_dependencies; i++) {
                    s.append("\"" + std::string(x.m_dependencies[i]) + "\"");
                    if (i < x.n_dependencies-1) {
                        s.append(",\n" + indtd);
                    };
                }
                dec_indent(); s.append("\n" + indtd);
            }
            s.append("]");
            s.append(",\n" + indtd);
            s.append("\"loaded_from_mod\": ");
            if (x.m_loaded_from_mod) {
                s.append("true");
            } else {
                s.append("false");
            }
            s.append(",\n" + indtd);
            s.append("\"intrinsic\": ");
            if (x.m_intrinsic) {
                s.append("true");
            } else {
                s.append("false");
            }
            dec_indent(); s.append("\n" + indtd);
            s.append("}");
            s.append(",\n" + indtd);
            append_location(s, x.base.base.loc.first, x.base.base.loc.last);
            dec_indent(); s.append("\n" + indtd);
            s.append("}");
        } else {
            ASR::JsonBaseVisitor<ASRJsonVisitor>::visit_Module(x);
        }
    }
};

std::string pickle_json(ASR::asr_t &asr, LocationManager &lm, bool show_intrinsic_modules) {
    ASRJsonVisitor v(lm);
    v.show_intrinsic_modules = show_intrinsic_modules;
    v.visit_asr(asr);
    return v.get_str();
}

std::string pickle_json(ASR::TranslationUnit_t &asr, LocationManager &lm, bool show_intrinsic_modules) {
    return pickle_json((ASR::asr_t &)asr, lm, show_intrinsic_modules);
}

}
