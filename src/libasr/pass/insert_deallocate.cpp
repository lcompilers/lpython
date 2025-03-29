#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/insert_deallocate.h>
#include<stack>


namespace LCompilers {

class InsertDeallocate: public ASR::CallReplacerOnExpressionsVisitor<InsertDeallocate>
{
    private:

        Allocator& al;
        std::stack<ASR::stmt_t*> implicitDeallocate_stmt_stack; // A stack to hold implicit_deallocate statement node due to nested visiting.

        void push_implicitDeallocate_into_stack(SymbolTable* symtab, Location &loc){
            Vec<ASR::expr_t*> default_storage_variables_to_deallocate = get_allocatable_default_storage_variables(symtab);
            if(default_storage_variables_to_deallocate.empty()){
                implicitDeallocate_stmt_stack.push(nullptr);
            } else {
                ASR::stmt_t* implicit_deallocation_node = ASRUtils::STMT(ASR::make_ImplicitDeallocate_t(
                    al, loc, default_storage_variables_to_deallocate.p, default_storage_variables_to_deallocate.size()));
                implicitDeallocate_stmt_stack.push(implicit_deallocation_node);
            }
        }

        inline bool is_deallocatable(ASR::symbol_t* s){
            if( ASR::is_a<ASR::Variable_t>(*s) && 
                ASR::is_a<ASR::Allocatable_t>(*ASRUtils::symbol_type(s)) && 
                (ASR::is_a<ASR::String_t>(*ASRUtils::type_get_past_allocatable(ASRUtils::symbol_type(s))) ||
                ASRUtils::is_array(ASRUtils::symbol_type(s)) ||
                ASRUtils::is_struct(*ASRUtils::symbol_type(s))) &&
                ASRUtils::symbol_intent(s) == ASRUtils::intent_local){
                return true;
            }
            return false;
        }

        // Returns vector of default-storage `Var` expressions that're deallocatable.
        Vec<ASR::expr_t*> get_allocatable_default_storage_variables(SymbolTable* symtab){ 
            Vec<ASR::expr_t*> allocatable_local_variables;
            allocatable_local_variables.reserve(al, 1);
            for(auto& itr: symtab->get_scope()) {
                if( is_deallocatable(itr.second) && 
                    ASRUtils::symbol_StorageType(itr.second) == ASR::storage_typeType::Default) {
                    allocatable_local_variables.push_back(al, ASRUtils::EXPR(
                        ASR::make_Var_t(al, itr.second->base.loc, itr.second)));
                }
            }
            return allocatable_local_variables;
        }        

        Vec<ASR::expr_t*> get_allocatable_save_storage_variables(SymbolTable* /*symtab*/){
            LCOMPILERS_ASSERT_MSG(false, "Not implemented");
            return Vec<ASR::expr_t*>();
        }

        // Insert `ImplicitDeallocate` before construct terminating statements.
        void visit_body_and_insert_ImplicitDeallocate(ASR::stmt_t** &m_body, size_t &n_body,
            const std::vector<ASR::stmtType> &construct_terminating_stmts = {ASR::stmtType::Return}){ 
            if(implicitDeallocate_stmt_stack.top() == nullptr){ // No variables to deallocate.
                return;
            }

            Vec<ASR::stmt_t*> new_body; // Final body after inserting finalization nodes.
            new_body.reserve(al, 1);
            bool return_or_exit_encounterd = false; // No need to insert finaliztion node once we encounter a `return` or `exit` stmt in signle body (Unreachable code).
            for(size_t i = 0; i < n_body; i++){
                for(ASR::stmtType statement_type : construct_terminating_stmts){
                    if( !return_or_exit_encounterd && 
                        (statement_type == m_body[i]->type)){
                        new_body.push_back(al, implicitDeallocate_stmt_stack.top());
                    }
                }
                if( ASR::is_a<ASR::Exit_t>(*m_body[i]) || 
                    ASR::is_a<ASR::Return_t>(*m_body[i])){
                    return_or_exit_encounterd = true; // Next statements are 'Unreachable Code'.
                }
                if(!return_or_exit_encounterd){
                    visit_stmt(*(m_body[i]));
                }
                new_body.push_back(al, m_body[i]);
            }
            m_body = new_body.p;
            n_body = new_body.size();
        }

        template <typename T>
        void insert_ImplicitDeallocate_at_end(T& x){
            LCOMPILERS_ASSERT_MSG(
                x.class_type == ASR::symbolType::Program  ||
                x.class_type == ASR::symbolType::Function ||
                x.class_type == ASR::symbolType::Block, "Only use with ASR::Program_t, Function_t or Block_t");
            if(implicitDeallocate_stmt_stack.top() == nullptr){
                return;
            }
            for(size_t i = 0; i < x.n_body; i++){
                if( ASR::is_a<ASR::Return_t>(*x.m_body[i]) || 
                    ASR::is_a<ASR::Exit_t>(*x.m_body[i])){
                    return; // already handled, and no need to insert at end.
                }
            }
            Vec<ASR::stmt_t*> new_body;
            new_body.from_pointer_n_copy(al, x.m_body, x.n_body);
            new_body.push_back(al, implicitDeallocate_stmt_stack.top());
            x.m_body = new_body.p;
            x.n_body = new_body.size();
        }



    public:

        InsertDeallocate(Allocator& al_) : al(al_) {}

        void visit_Function(const ASR::Function_t& x) {
            ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
            push_implicitDeallocate_into_stack(xx.m_symtab, xx.base.base.loc);
            for (auto &a : x.m_symtab->get_scope()) {
                visit_symbol(*a.second);
            }
            visit_body_and_insert_ImplicitDeallocate(xx.m_body,xx.n_body);
            insert_ImplicitDeallocate_at_end(xx);
            implicitDeallocate_stmt_stack.pop(); 
        }

        void visit_Program(const ASR::Program_t& x) {
            ASR::Program_t &xx = const_cast<ASR::Program_t&>(x);
            push_implicitDeallocate_into_stack(xx.m_symtab, xx.base.base.loc);

            for (auto &a : x.m_symtab->get_scope()) {
                visit_symbol(*a.second);
            }
            visit_body_and_insert_ImplicitDeallocate(xx.m_body, xx.n_body);
            insert_ImplicitDeallocate_at_end(xx);
            implicitDeallocate_stmt_stack.pop(); 
        }

        void visit_Block(const ASR::Block_t& x){
            ASR::Block_t& xx = const_cast<ASR::Block_t&>(x);
            push_implicitDeallocate_into_stack(xx.m_symtab, xx.base.base.loc);
            for (auto &a : x.m_symtab->get_scope()) {
                visit_symbol(*a.second);
            }
            visit_body_and_insert_ImplicitDeallocate(xx.m_body, xx.n_body,{ASR::stmtType::Return, ASR::stmtType::Exit});
            insert_ImplicitDeallocate_at_end(xx);
            implicitDeallocate_stmt_stack.pop(); 
        }

        void visit_If(const ASR::If_t& x){
            ASR::If_t &xx = const_cast<ASR::If_t&>(x);
            visit_body_and_insert_ImplicitDeallocate(xx.m_body, xx.n_body);
            visit_body_and_insert_ImplicitDeallocate(xx.m_orelse, xx.n_orelse);
        }
        
        void visit_WhileLoop(const ASR::WhileLoop_t &x){
            ASR::WhileLoop_t &xx = const_cast<ASR::WhileLoop_t&>(x);
            visit_body_and_insert_ImplicitDeallocate(xx.m_body, xx.n_body);
            visit_body_and_insert_ImplicitDeallocate(xx.m_orelse, xx.n_orelse);
        }

        void visit_DoLoop(const ASR::DoLoop_t &x){
            ASR::DoLoop_t &xx = const_cast<ASR::DoLoop_t&>(x);
            visit_body_and_insert_ImplicitDeallocate(xx.m_body, xx.n_body);
            visit_body_and_insert_ImplicitDeallocate(xx.m_orelse, xx.n_orelse);
        }

};

void pass_insert_deallocate(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &/*pass_options*/) {
    InsertDeallocate v(al);
    v.visit_TranslationUnit(unit);
}


} // namespace LCompilers
