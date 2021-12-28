namespace LFortran {

namespace ASR {
    template <class Derived>
    class StatementWalkVisitor : public ASR::BaseWalkVisitor<Derived>
    {
    public:
        Allocator &al;
        Vec<ASR::stmt_t*> stmts;
        bool asr_changed;

        StatementWalkVisitor(Allocator &al) : al{al}, asr_changed{false} {
            stmts.n = 0;
        }

        void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
            Vec<ASR::stmt_t*> body;
            body.reserve(al, n_body);
            for (size_t i=0; i<n_body; i++) {
                // Not necessary after we check it after each visit_stmt in every
                // visitor method:
                this->visit_stmt(*m_body[i]);
                if (stmts.size() > 0) {
                    asr_changed = true;
                    for (size_t j=0; j<stmts.size(); j++) {
                        body.push_back(al, stmts[j]);
                    }
                    stmts.n = 0;
                } else {
                    body.push_back(al, m_body[i]);
                }
            }
            m_body = body.p;
            n_body = body.size();
        }

        void visit_Program(const ASR::Program_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::Program_t &xx = const_cast<ASR::Program_t&>(x);
            transform_stmts(xx.m_body, xx.n_body);

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
            transform_stmts(xx.m_body, xx.n_body);
        }

        void visit_Function(const ASR::Function_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
            transform_stmts(xx.m_body, xx.n_body);
        }

        void visit_WhileLoop(const ASR::WhileLoop_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::WhileLoop_t &xx = const_cast<ASR::WhileLoop_t&>(x);
            transform_stmts(xx.m_body, xx.n_body);
        }

        void visit_DoLoop(const ASR::DoLoop_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::DoLoop_t &xx = const_cast<ASR::DoLoop_t&>(x);
            transform_stmts(xx.m_body, xx.n_body);
        }
    };
} // namespace ASR

} // namespace LFortran
