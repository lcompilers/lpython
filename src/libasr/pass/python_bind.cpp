#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/replace_arr_slice.h>

namespace LCompilers {
    void pass_python_bind(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options) {
        bool bindpython_used = false;
        for (auto &item : unit.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(item.second);
                if (ASRUtils::get_FunctionType(f)->m_abi == ASR::abiType::BindPython) {
                    bindpython_used = true;
                }
            }
            else if (ASR::is_a<ASR::Module_t>(*item.second)) {
                ASR::Module_t *module = ASR::down_cast<ASR::Module_t>(item.second);
                for (auto &module_item: module->m_symtab->get_scope() ) {
                    if (ASR::is_a<ASR::Function_t>(*module_item.second)) {
                        ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(module_item.second);
                        if (ASRUtils::get_FunctionType(f)->m_abi == ASR::abiType::BindPython) {
                            bindpython_used = true;
                            {
                                LCOMPILERS_ASSERT_MSG(f->n_args == 0, "BindPython with arguments not supported yet");
                                LCOMPILERS_ASSERT_MSG(f->m_return_var == nullptr, "BindPython with return types not supported yet");

                                Vec<ASR::stmt_t*> body;
                                body.reserve(al, 1);
                                Str s;
                                
                                /*
                                    if (!Py_IsInitialized()) {
                                        Py_Initialize();
                                        int pS = PyRun_SimpleString("import sys; sys.path.append('.')");
                                    }
                                */
                                ASR::symbol_t *sym_Py_IsInitialized = module->m_symtab->get_symbol("Py_IsInitialized");
                                ASR::symbol_t *sym_Py_Initialize = module->m_symtab->get_symbol("Py_Initialize");
                                LCOMPILERS_ASSERT(sym_Py_IsInitialized)
                                ASR::ttype_t *i4_type = ASRUtils::TYPE(ASR::make_Integer_t(al, f->base.base.loc, 4));
                                ASR::asr_t *call_Py_IsInitialized = ASRUtils::make_FunctionCall_t_util(al, f->base.base.loc, sym_Py_IsInitialized, nullptr, nullptr, 0, i4_type, nullptr, nullptr);
                                ASR::asr_t * if_cond = ASR::make_IntegerCompare_t(al, f->base.base.loc, ASRUtils::EXPR(call_Py_IsInitialized), ASR::cmpopType::Eq, ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, f->base.base.loc, 0, i4_type)), i4_type, nullptr);
                                Vec<ASR::stmt_t*> if_body;
                                if_body.reserve(al, 2);
                                ASR::asr_t *call_Py_Initialize = ASRUtils::make_SubroutineCall_t_util(al, f->base.base.loc, sym_Py_Initialize, nullptr, nullptr, 0, nullptr, nullptr, false, false);
                                if_body.push_back(al, ASRUtils::STMT(call_Py_Initialize));

                                ASR::symbol_t *sym_PyRun_SimpleString = module->m_symtab->get_symbol("PyRun_SimpleString");
                                Vec<ASR::call_arg_t> args_PyRun_SimpleString;
                                s.from_str(al, "import sys; sys.path.append(\".\")");
                                args_PyRun_SimpleString.reserve(al, 1);
                                ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, f->base.base.loc, 1, s.size(), nullptr));
                                args_PyRun_SimpleString.push_back(al, {f->base.base.loc, ASRUtils::EXPR(ASR::make_StringConstant_t(al, f->base.base.loc, s.c_str(al), str_type))});
                                s.from_str(al, "pS");
                                ASR::asr_t *pS = ASR::make_Variable_t(al, f->base.base.loc, f->m_symtab, s.c_str(al),
                                                        nullptr, 0, ASRUtils::intent_local, nullptr, nullptr,
                                                        ASR::storage_typeType::Default, i4_type,
                                                        nullptr, ASR::abiType::BindC,
                                                        ASR::Public, ASR::presenceType::Required, false);
                                ASR::expr_t *pS_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f->base.base.loc, ASR::down_cast<ASR::symbol_t>(pS)));
                                f->m_symtab->add_symbol(std::string("pS"), ASR::down_cast<ASR::symbol_t>(pS));
                                if_body.push_back(al, ASRUtils::STMT(
                                    ASR::make_Assignment_t(al, f->base.base.loc, pS_ref, 
                                        ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f->base.base.loc, sym_PyRun_SimpleString, nullptr, args_PyRun_SimpleString.p, args_PyRun_SimpleString.n, i4_type, nullptr, nullptr)),
                                        nullptr)));

                                body.push_back(al, ASRUtils::STMT(ASR::make_If_t(al, f->base.base.loc, ASRUtils::EXPR(if_cond), if_body.p, if_body.n, nullptr, 0)));

                                /*
                                    void *pName = PyUnicode_FromString(module_name);
                                    void *pModule = PyImport_Import(pName);
                                    void *pFunc = PyObject_GetAttrString(pModule, func_name);

                                    void *pArgs = PyTuple_New(0);
                                    void *pReturn = PyObject_CallObject(pFunc, pArgs);
                                    
                                    Py_DecRef(pName);
                                    Py_DecRef(pArgs);
                                    Py_DecRef(pReturn);
                                */
                                ASR::ttype_t *ptr_t = ASRUtils::TYPE(ASR::make_CPtr_t(al, f->base.base.loc));

                                ASR::symbol_t *sym_PyUnicode_FromString = module->m_symtab->get_symbol("PyUnicode_FromString");
                                Vec<ASR::call_arg_t> args_PyUnicode_FromString;
                                s.from_str(al, f->m_module_file);
                                args_PyUnicode_FromString.reserve(al, 1);
                                str_type = ASRUtils::TYPE(ASR::make_Character_t(al, f->base.base.loc, 1, s.size(), nullptr));
                                args_PyUnicode_FromString.push_back(al, {f->base.base.loc, ASRUtils::EXPR(ASR::make_StringConstant_t(al, f->base.base.loc, s.c_str(al), str_type))});
                                s.from_str(al, "pName");
                                ASR::asr_t *pName = ASR::make_Variable_t(al, f->base.base.loc, f->m_symtab, s.c_str(al),
                                                        nullptr, 0, ASRUtils::intent_local, nullptr, nullptr,
                                                        ASR::storage_typeType::Default, ptr_t,
                                                        nullptr, ASR::abiType::BindC,
                                                        ASR::Public, ASR::presenceType::Required, false);
                                ASR::expr_t *pName_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f->base.base.loc, ASR::down_cast<ASR::symbol_t>(pName)));
                                f->m_symtab->add_symbol(std::string("pName"), ASR::down_cast<ASR::symbol_t>(pName));
                                body.push_back(al, ASRUtils::STMT(
                                    ASR::make_Assignment_t(al, f->base.base.loc, pName_ref, 
                                        ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f->base.base.loc, sym_PyUnicode_FromString, nullptr, args_PyUnicode_FromString.p, args_PyUnicode_FromString.n, ptr_t, nullptr, nullptr)),
                                        nullptr)));
                                
                                ASR::symbol_t *sym_PyImport_Import = module->m_symtab->get_symbol("PyImport_Import");
                                Vec<ASR::call_arg_t> args_PyImport_Import;
                                args_PyImport_Import.reserve(al, 1);
                                args_PyImport_Import.push_back(al, {f->base.base.loc, pName_ref});
                                s.from_str(al, "pModule");
                                ASR::asr_t *pModule = ASR::make_Variable_t(al, f->base.base.loc, f->m_symtab, s.c_str(al),
                                                        nullptr, 0, ASRUtils::intent_local, nullptr, nullptr,
                                                        ASR::storage_typeType::Default, ptr_t,
                                                        nullptr, ASR::abiType::BindC,
                                                        ASR::Public, ASR::presenceType::Required, false);
                                ASR::expr_t *pModule_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f->base.base.loc, ASR::down_cast<ASR::symbol_t>(pModule)));
                                f->m_symtab->add_symbol(std::string("pModule"), ASR::down_cast<ASR::symbol_t>(pModule));
                                body.push_back(al, ASRUtils::STMT(
                                    ASR::make_Assignment_t(al, f->base.base.loc, pModule_ref, 
                                        ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f->base.base.loc, sym_PyImport_Import, nullptr, args_PyImport_Import.p, args_PyImport_Import.n, ptr_t, nullptr, nullptr)),
                                        nullptr)));
                                
                                ASR::symbol_t *sym_PyObject_GetAttrString = module->m_symtab->get_symbol("PyObject_GetAttrString");
                                Vec<ASR::call_arg_t> args_PyObject_GetAttrString;
                                s.from_str(al, f->m_module_file);
                                args_PyObject_GetAttrString.reserve(al, 2);
                                args_PyObject_GetAttrString.push_back(al, {f->base.base.loc, pModule_ref});
                                s.from_str(al, f->m_name);
                                str_type = ASRUtils::TYPE(ASR::make_Character_t(al, f->base.base.loc, 1, s.size(), nullptr));
                                args_PyObject_GetAttrString.push_back(al, {f->base.base.loc, ASRUtils::EXPR(ASR::make_StringConstant_t(al, f->base.base.loc, s.c_str(al), str_type))});
                                s.from_str(al, "pFunc");
                                ASR::asr_t *pFunc = ASR::make_Variable_t(al, f->base.base.loc, f->m_symtab, s.c_str(al),
                                                        nullptr, 0, ASRUtils::intent_local, nullptr, nullptr,
                                                        ASR::storage_typeType::Default, ptr_t,
                                                        nullptr, ASR::abiType::BindC,
                                                        ASR::Public, ASR::presenceType::Required, false);
                                ASR::expr_t *pFunc_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f->base.base.loc, ASR::down_cast<ASR::symbol_t>(pFunc)));
                                f->m_symtab->add_symbol(std::string("pFunc"), ASR::down_cast<ASR::symbol_t>(pFunc));
                                body.push_back(al, ASRUtils::STMT(
                                    ASR::make_Assignment_t(al, f->base.base.loc, pFunc_ref, 
                                        ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f->base.base.loc, sym_PyObject_GetAttrString, nullptr, args_PyObject_GetAttrString.p, args_PyObject_GetAttrString.n, ptr_t, nullptr, nullptr)),
                                        nullptr)));

                                ASR::symbol_t *sym_PyTuple_New = module->m_symtab->get_symbol("PyTuple_New");
                                Vec<ASR::call_arg_t> args_PyTuple_New;
                                args_PyTuple_New.reserve(al, 1);
                                args_PyTuple_New.push_back(al, {f->base.base.loc, ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, f->base.base.loc, f->n_args, i4_type))});
                                s.from_str(al, "pArgs");
                                ASR::asr_t *pArgs = ASR::make_Variable_t(al, f->base.base.loc, f->m_symtab, s.c_str(al),
                                                        nullptr, 0, ASRUtils::intent_local, nullptr, nullptr,
                                                        ASR::storage_typeType::Default, ptr_t,
                                                        nullptr, ASR::abiType::BindC,
                                                        ASR::Public, ASR::presenceType::Required, false);
                                ASR::expr_t *pArgs_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f->base.base.loc, ASR::down_cast<ASR::symbol_t>(pArgs)));
                                f->m_symtab->add_symbol(std::string("pArgs"), ASR::down_cast<ASR::symbol_t>(pArgs));
                                body.push_back(al, ASRUtils::STMT(
                                    ASR::make_Assignment_t(al, f->base.base.loc, pArgs_ref, 
                                        ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f->base.base.loc, sym_PyTuple_New, nullptr, args_PyTuple_New.p, args_PyTuple_New.n, ptr_t, nullptr, nullptr)),
                                        nullptr)));
                                
                                /*
                                    Arg's type conversions goes here
                                */

                                ASR::symbol_t *sym_PyObject_CallObject = module->m_symtab->get_symbol("PyObject_CallObject");
                                Vec<ASR::call_arg_t> args_PyObject_CallObject;
                                args_PyObject_CallObject.reserve(al, 2);
                                args_PyObject_CallObject.push_back(al, {f->base.base.loc, pFunc_ref});
                                args_PyObject_CallObject.push_back(al, {f->base.base.loc, pArgs_ref});
                                s.from_str(al, "pReturn");
                                ASR::asr_t *pReturn = ASR::make_Variable_t(al, f->base.base.loc, f->m_symtab, s.c_str(al),
                                                        nullptr, 0, ASRUtils::intent_local, nullptr, nullptr,
                                                        ASR::storage_typeType::Default, ptr_t,
                                                        nullptr, ASR::abiType::BindC,
                                                        ASR::Public, ASR::presenceType::Required, false);
                                ASR::expr_t *pReturn_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f->base.base.loc, ASR::down_cast<ASR::symbol_t>(pReturn)));
                                f->m_symtab->add_symbol(std::string("pReturn"), ASR::down_cast<ASR::symbol_t>(pReturn));
                                body.push_back(al, ASRUtils::STMT(
                                    ASR::make_Assignment_t(al, f->base.base.loc, pReturn_ref, 
                                        ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f->base.base.loc, sym_PyObject_CallObject, nullptr, args_PyObject_CallObject.p, args_PyObject_CallObject.n, ptr_t, nullptr, nullptr)),
                                        nullptr)));
                                
                                /*
                                    Return type conversion goes here
                                */

                                ASR::symbol_t *sym_Py_DecRef = module->m_symtab->get_symbol("Py_DecRef");

                                Vec<ASR::call_arg_t> args_Py_DecRef;
                                args_Py_DecRef.reserve(al, 1);                                
                                args_Py_DecRef.push_back(al, {f->base.base.loc, pName_ref});
                                body.push_back(al, ASRUtils::STMT(ASRUtils::make_SubroutineCall_t_util(al, f->base.base.loc, sym_Py_DecRef, nullptr, args_Py_DecRef.p, args_Py_DecRef.n, nullptr, nullptr, false, false)));

                                Vec<ASR::call_arg_t> args_Py_DecRef2;
                                args_Py_DecRef2.reserve(al, 1);
                                args_Py_DecRef2.push_back(al, {f->base.base.loc, pArgs_ref});
                                body.push_back(al, ASRUtils::STMT(ASRUtils::make_SubroutineCall_t_util(al, f->base.base.loc, sym_Py_DecRef, nullptr, args_Py_DecRef2.p, args_Py_DecRef2.n, nullptr, nullptr, false, false)));

                                Vec<ASR::call_arg_t> args_Py_DecRef3;
                                args_Py_DecRef3.reserve(al, 1);
                                args_Py_DecRef3.push_back(al, {f->base.base.loc, pReturn_ref});
                                body.push_back(al, ASRUtils::STMT(ASRUtils::make_SubroutineCall_t_util(al, f->base.base.loc, sym_Py_DecRef, nullptr, args_Py_DecRef3.p, args_Py_DecRef3.n, nullptr, nullptr, false, false)));
                                
                                // reassignment
                                f->m_body = body.p;
                                f->n_body = body.n;
                                ASRUtils::get_FunctionType(f)->m_abi = ASR::abiType::BindC;
                                ASRUtils::get_FunctionType(f)->m_deftype = ASR::deftypeType::Implementation;
                            }
                        }
                    }
                }
            }

            if (bindpython_used) { 
                break;
            }
        }
    }
}
