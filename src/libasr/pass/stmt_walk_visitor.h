#include <libasr/pass/pass_utils.h>

namespace LFortran {

namespace ASR {
    template <class Derived>
    class StatementWalkVisitor : public PassUtils::PassVisitor<Derived>
    {

    private:
        Derived& self() { return static_cast<Derived&>(*this); }

    public:
        Allocator &al;
        Vec<ASR::stmt_t*> stmts;

        StatementWalkVisitor(Allocator &al) : al{al} {
            stmts.n = 0;
        }

        void visit_Program(const ASR::Program_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::Program_t &xx = const_cast<ASR::Program_t&>(x);
            self().transform_stmts(xx.m_body, xx.n_body, al, stmts);

            // Transform nested functions and subroutines
            for (auto &item : x.m_symtab->scope) {
                if (is_a<ASR::Subroutine_t>(*item.second)) {
                    ASR::Subroutine_t *s = down_cast<ASR::Subroutine_t>(item.second);
                    visit_Subroutine(*s);
                }
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *s = down_cast<ASR::Function_t>(item.second);
                    visit_Function(*s);
                }
            }
        }

        void visit_Subroutine(const ASR::Subroutine_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::Subroutine_t &xx = const_cast<ASR::Subroutine_t&>(x);
            self().transform_stmts(xx.m_body, xx.n_body, al, stmts);
        }

        void visit_Function(const ASR::Function_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
            self().transform_stmts(xx.m_body, xx.n_body, al, stmts);
        }

        void visit_WhileLoop(const ASR::WhileLoop_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::WhileLoop_t &xx = const_cast<ASR::WhileLoop_t&>(x);
            self().transform_stmts(xx.m_body, xx.n_body, al, stmts);
        }

        void visit_DoLoop(const ASR::DoLoop_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::DoLoop_t &xx = const_cast<ASR::DoLoop_t&>(x);
            self().transform_stmts(xx.m_body, xx.n_body, al, stmts);
        }
    };
} // namespace ASR

} // namespace LFortran
