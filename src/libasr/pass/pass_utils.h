#ifndef LFORTRAN_PASS_UTILS_H
#define LFORTRAN_PASS_UTILS_H

#include <libasr/asr.h>
#include <libasr/containers.h>

namespace LFortran {

    namespace PassUtils {

        bool is_array(ASR::expr_t* x);

        void get_dim_rank(ASR::ttype_t* x_type, ASR::dimension_t*& m_dims, int& n_dims);

        ASR::ttype_t* set_dim_rank(ASR::ttype_t* x_type, ASR::dimension_t*& m_dims, int& n_dims,
                                    bool create_new=false, Allocator* al=nullptr);

        int get_rank(ASR::expr_t* x);

        ASR::expr_t* create_array_ref(ASR::expr_t* arr_expr, Vec<ASR::expr_t*>& idx_vars, Allocator& al);

        ASR::expr_t* create_array_ref(ASR::symbol_t* arr, Vec<ASR::expr_t*>& idx_vars, Allocator& al,
                                      const Location& loc, ASR::ttype_t* _type);

        void create_idx_vars(Vec<ASR::expr_t*>& idx_vars, int n_dims, const Location& loc,
                             Allocator& al, SymbolTable*& current_scope, std::string suffix="_k");

        ASR::expr_t* get_bound(ASR::expr_t* arr_expr, int dim, std::string bound,
                                Allocator& al, ASR::TranslationUnit_t& unit,
                                const std::string &rl_path,
                                SymbolTable*& current_scope);


        ASR::stmt_t* get_flipsign(ASR::expr_t* arg0, ASR::expr_t* arg1,
                                  Allocator& al, ASR::TranslationUnit_t& unit,
                                  const std::string &rl_path,
                                  SymbolTable*& current_scope,
                                  const std::function<void (const std::string &, const Location &)> err);

        ASR::expr_t* to_int32(ASR::expr_t* x, ASR::ttype_t* int32type, Allocator& al);

        bool is_slice_present(const ASR::ArrayRef_t& x);

        bool is_slice_present(const ASR::expr_t* x);

        template <class Derived>
        class PassVisitor: public ASR::BaseWalkVisitor<Derived> {

            private:

                Derived& self() { return static_cast<Derived&>(*this); }

            public:

                bool asr_changed;

                void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body,
                                     Allocator& al, Vec<ASR::stmt_t*>& pass_result) {
                    Vec<ASR::stmt_t*> body;
                    body.reserve(al, n_body);
                    for (size_t i=0; i<n_body; i++) {
                        // Not necessary after we check it after each visit_stmt in every
                        // visitor method:
                        pass_result.n = 0;
                        self().visit_stmt(*m_body[i]);
                        if (pass_result.size() > 0) {
                            asr_changed = true;
                            for (size_t j=0; j < pass_result.size(); j++) {
                                body.push_back(al, pass_result[j]);
                            }
                            pass_result.n = 0;
                        } else {
                            body.push_back(al, m_body[i]);
                        }
                    }
                    m_body = body.p;
                    n_body = body.size();
                }

        };

    }

} // namespace LFortran

#endif // LFORTRAN_PASS_UTILS_H
