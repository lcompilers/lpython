
#ifndef LFORTRAN_SEMANTICS_ASR_SYMBOLTABLE_VISITOR_H
#define LFORTRAN_SEMANTICS_ASR_SYMBOLTABLE_VISITOR_H

#include <map>

#include <lfortran/asr.h>
#include <lfortran/ast.h>

namespace LFortran {
class SymbolTableVisitor : public AST::BaseVisitor<SymbolTableVisitor> {
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
  ASR::asr_t *asr;
  Allocator &al;
  SymbolTable *current_scope;
  SymbolTable *global_scope;
  std::map<std::string, std::vector<std::string>> generic_procedures;
  std::map<std::string, std::map<std::string, std::string>> class_procedures;
  std::string dt_name;
  ASR::accessType dflt_access = ASR::Public;
  ASR::presenceType dflt_presence = ASR::presenceType::Required;
  std::map<std::string, ASR::accessType> assgnd_access;
  std::map<std::string, ASR::presenceType> assgnd_presence;
  Vec<char *> current_module_dependencies;
  bool in_module = false;
  bool is_interface = false;
  std::vector<std::string> current_procedure_args;

  SymbolTableVisitor(Allocator &al, SymbolTable *symbol_table)
      : al{al}, current_scope{symbol_table} {}

  ASR::symbol_t *resolve_symbol(const Location &loc, const char *id);
  void visit_TranslationUnit(const AST::TranslationUnit_t &x);
  void visit_Module(const AST::Module_t &x);
  void visit_Program(const AST::Program_t &x);
  void visit_Subroutine(const AST::Subroutine_t &x);
  AST::AttrType_t *find_return_type(AST::decl_attribute_t **attributes,
                                    size_t n, const Location &loc);

  void visit_Function(const AST::Function_t &x);
  void visit_StrOp(const AST::StrOp_t &x);
  void visit_UnaryOp(const AST::UnaryOp_t &x);
  void visit_BoolOp(const AST::BoolOp_t &x);
  void visit_Compare(const AST::Compare_t &x);
  void visit_BinOp(const AST::BinOp_t &x);
  void visit_String(const AST::String_t &x);
  void visit_Logical(const AST::Logical_t &x);
  void visit_Complex(const AST::Complex_t &x);
  void process_dims(Allocator &al, Vec<ASR::dimension_t> &dims,
                    AST::dimension_t *m_dim, size_t n_dim);

  void visit_Declaration(const AST::Declaration_t &x);
  Vec<ASR::expr_t *> visit_expr_list(AST::fnarg_t *ast_list, size_t n);
  void visit_FuncCallOrArray(const AST::FuncCallOrArray_t &x);
  void visit_DerivedType(const AST::DerivedType_t &x);
  void visit_InterfaceProc(const AST::InterfaceProc_t &x);
  void visit_DerivedTypeProc(const AST::DerivedTypeProc_t &x);
  void visit_Interface(const AST::Interface_t &x);
  void add_generic_procedures();
  void add_class_procedures();
  void visit_Use(const AST::Use_t &x);
  void visit_Real(const AST::Real_t &x);
  ASR::asr_t *resolve_variable(const Location &loc, const char *id);
  void visit_Name(const AST::Name_t &x);
  void visit_Num(const AST::Num_t &x);
  void visit_Parenthesis(const AST::Parenthesis_t &x);
};

} // namespace LFortran

#endif /* LFORTRAN_SEMANTICS_ASR_SYMBOLTABLE_VISITOR_H */
