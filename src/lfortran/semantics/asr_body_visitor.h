#ifndef LFORTRAN_SEMANTICS_ASR_BODY_VISITOR_H
#define LFORTRAN_SEMANTICS_ASR_BODY_VISITOR_H

#include <map>

#include <lfortran/asr.h>
#include <lfortran/ast.h>

namespace LFortran {
class BodyVisitor : public AST::BaseVisitor<BodyVisitor> {
private:
  std::map<std::string, std::string> intrinsic_procedures = {
      {"kind", "lfortran_intrinsic_kind"},
      {"selected_int_kind", "lfortran_intrinsic_kind"},
      {"selected_real_kind", "lfortran_intrinsic_kind"},
      {"size", "lfortran_intrinsic_array"},
      {"lbound", "lfortran_intrinsic_array"},
      {"ubound", "lfortran_intrinsic_array"},
      {"min", "lfortran_intrinsic_array"},
      {"max", "lfortran_intrinsic_array"},
      {"allocated", "lfortran_intrinsic_array"},
      {"minval", "lfortran_intrinsic_array"},
      {"maxval", "lfortran_intrinsic_array"},
      {"real", "lfortran_intrinsic_array"},
      {"sum", "lfortran_intrinsic_array"},
      {"abs", "lfortran_intrinsic_array"}};

public:
  Allocator &al;
  ASR::asr_t *asr, *tmp;
  SymbolTable *current_scope;
  Vec<ASR::stmt_t*> *current_body;
  ASR::Module_t *current_module = nullptr;

  BodyVisitor(Allocator &al, ASR::asr_t *unit) : al{al}, asr{unit} {}

  void visit_TranslationUnit(const AST::TranslationUnit_t &x);
  void visit_Declaration(const AST::Declaration_t & /* x */){
      // Already visited this AST node in the SymbolTableVisitor
  };
  void visit_Open(const AST::Open_t &x);
  void visit_Close(const AST::Close_t &x);
  void create_read_write_ASR_node(const AST::stmt_t &read_write_stmt,
                                  AST::stmtType _type);
  void visit_Write(const AST::Write_t &x);
  void visit_Read(const AST::Read_t &x);
  void visit_Associate(const AST::Associate_t &x);
  void visit_AssociateBlock(const AST::AssociateBlock_t& x);
  void visit_Allocate(const AST::Allocate_t &x);
  ASR::stmt_t *create_implicit_deallocate(const Location &loc);
  void visit_Deallocate(const AST::Deallocate_t &x);
  void visit_Return(const AST::Return_t &x);
  void visit_case_stmt(const AST::case_stmt_t &x);
  void visit_Select(const AST::Select_t &x);
  void visit_Module(const AST::Module_t &x);
  void visit_Program(const AST::Program_t &x);
  ASR::stmt_t *create_implicit_deallocate_subrout_call(ASR::stmt_t *x);
  void visit_Subroutine(const AST::Subroutine_t &x);
  void visit_Function(const AST::Function_t &x);
  void visit_Assignment(const AST::Assignment_t &x);
  Vec<ASR::expr_t *> visit_expr_list(AST::fnarg_t *ast_list, size_t n);
  void visit_SubroutineCall(const AST::SubroutineCall_t &x);
  int select_generic_procedure(const Vec<ASR::expr_t *> &args,
                               const ASR::GenericProcedure_t &p, Location loc);
  bool argument_types_match(const Vec<ASR::expr_t *> &args,
                            const ASR::Subroutine_t &sub);
  bool types_equal(const ASR::ttype_t &a, const ASR::ttype_t &b);
  void visit_Compare(const AST::Compare_t &x);
  void visit_BoolOp(const AST::BoolOp_t &x);
  void visit_BinOp(const AST::BinOp_t &x);
  void visit_StrOp(const AST::StrOp_t &x);
  void visit_UnaryOp(const AST::UnaryOp_t &x);
  ASR::asr_t *resolve_variable(const Location &loc, const char *id);
  ASR::asr_t *getDerivedRef_t(const Location &loc, ASR::asr_t *v_var,
                              ASR::symbol_t *member);
  ASR::asr_t *resolve_variable2(const Location &loc, const char *id,
                                const char *derived_type_id,
                                SymbolTable *&scope);
  ASR::symbol_t *resolve_deriv_type_proc(const Location &loc, const char *id,
                                         const char *derived_type_id,
                                         SymbolTable *&scope);
  void visit_Name(const AST::Name_t &x);
  void visit_FuncCallOrArray(const AST::FuncCallOrArray_t &x);
  void visit_Num(const AST::Num_t &x);
  void visit_Parenthesis(const AST::Parenthesis_t &x);
  void visit_Logical(const AST::Logical_t &x);
  void visit_String(const AST::String_t &x);
  void visit_Real(const AST::Real_t &x);
  void visit_Complex(const AST::Complex_t &x);
  void visit_ArrayInitializer(const AST::ArrayInitializer_t &x);
  void visit_Print(const AST::Print_t &x);
  void visit_If(const AST::If_t &x);
  void visit_WhileLoop(const AST::WhileLoop_t &x);
  void visit_ImpliedDoLoop(const AST::ImpliedDoLoop_t &x);
  void visit_DoLoop(const AST::DoLoop_t &x);
  void visit_DoConcurrentLoop(const AST::DoConcurrentLoop_t &x);
  void visit_Exit(const AST::Exit_t &x);
  void visit_Cycle(const AST::Cycle_t &x);
  void visit_Continue(const AST::Continue_t & /*x*/);
  void visit_Stop(const AST::Stop_t &x);
  void visit_ErrorStop(const AST::ErrorStop_t &x);
};
} // namespace LFortran

#endif /* LFORTRAN_SEMANTICS_ASR_BODY_VISITOR_H */
