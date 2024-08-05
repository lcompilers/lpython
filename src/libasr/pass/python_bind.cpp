#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/replace_arr_slice.h>


namespace LCompilers {
ASR::expr_t *cpython_to_native(Allocator &al, ASR::expr_t *exp, ASR::ttype_t *type, const ASR::Function_t &f,
                                const SymbolTable &parent_scope) {
    ASR::ttype_t *i1_type = ASRUtils::TYPE(ASR::make_Integer_t(al, f.base.base.loc, 1));
    ASR::ttype_t *i1ptr_type = ASRUtils::TYPE(ASR::make_Pointer_t(al, f.base.base.loc, i1_type));
    ASR::ttype_t *i4_type = ASRUtils::TYPE(ASR::make_Integer_t(al, f.base.base.loc, 4));
    ASR::ttype_t *i8_type = ASRUtils::TYPE(ASR::make_Integer_t(al, f.base.base.loc, 8));
    ASR::ttype_t *u8_type = ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al, f.base.base.loc, 8));
    ASR::ttype_t *f8_type = ASRUtils::TYPE(ASR::make_Real_t(al, f.base.base.loc, 8));
    ASR::ttype_t *ptr_t = ASRUtils::TYPE(ASR::make_CPtr_t(al, f.base.base.loc));

    ASR::expr_t *conv_result = nullptr;
    if (type->type == ASR::ttypeType::Integer) {
        ASR::symbol_t *sym_PyLong_AsLongLong = parent_scope.get_symbol("PyLong_AsLongLong");
        Vec<ASR::call_arg_t> args_PyLong_AsLongLong;
        args_PyLong_AsLongLong.reserve(al, 1);
        args_PyLong_AsLongLong.push_back(al, {f.base.base.loc, exp});
        conv_result = ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc,
                            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                                sym_PyLong_AsLongLong, nullptr, args_PyLong_AsLongLong.p, args_PyLong_AsLongLong.n,
                                i8_type, nullptr, nullptr)),
                            ASR::IntegerToInteger, type, nullptr));
    } else if (type->type == ASR::ttypeType::UnsignedInteger) {
        ASR::symbol_t *sym_PyLong_AsUnsignedLongLong = parent_scope.get_symbol("PyLong_AsUnsignedLongLong");
        Vec<ASR::call_arg_t> args_PyLong_AsUnsignedLongLong;
        args_PyLong_AsUnsignedLongLong.reserve(al, 1);
        args_PyLong_AsUnsignedLongLong.push_back(al, {f.base.base.loc, exp});
        conv_result = ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc, 
                            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                                sym_PyLong_AsUnsignedLongLong, nullptr, args_PyLong_AsUnsignedLongLong.p,
                                args_PyLong_AsUnsignedLongLong.n, u8_type, nullptr, nullptr)),
                            ASR::UnsignedIntegerToUnsignedInteger, type, nullptr));
    } else if (type->type == ASR::ttypeType::Real) {
        ASR::symbol_t *sym_PyFloat_AsDouble = parent_scope.get_symbol("PyFloat_AsDouble");
        Vec<ASR::call_arg_t> args_PyFloat_AsDouble;
        args_PyFloat_AsDouble.reserve(al, 1);
        args_PyFloat_AsDouble.push_back(al, {f.base.base.loc, exp});
        conv_result = ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc,
                            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                                sym_PyFloat_AsDouble, nullptr, args_PyFloat_AsDouble.p, args_PyFloat_AsDouble.n,
                                f8_type, nullptr, nullptr)),
                            ASR::RealToReal, type, nullptr));
    } else if (type->type == ASR::ttypeType::Logical) {
        ASR::symbol_t *sym_PyObject_IsTrue = parent_scope.get_symbol("PyObject_IsTrue");
        Vec<ASR::call_arg_t> args_PyObject_IsTrue;
        args_PyObject_IsTrue.reserve(al, 1);
        args_PyObject_IsTrue.push_back(al, {f.base.base.loc, exp});
        conv_result = ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc, 
                            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                                sym_PyObject_IsTrue, nullptr, args_PyObject_IsTrue.p, args_PyObject_IsTrue.n,
                                i4_type, nullptr, nullptr)),
                            ASR::IntegerToLogical, type, nullptr));
    } else if (type->type == ASR::ttypeType::Character) {
        ASR::symbol_t *sym_PyUnicode_AsUTF8AndSize = parent_scope.get_symbol("PyUnicode_AsUTF8AndSize");
        Vec<ASR::call_arg_t> args_PyUnicode_AsUTF8AndSize;
        args_PyUnicode_AsUTF8AndSize.reserve(al, 1);
        args_PyUnicode_AsUTF8AndSize.push_back(al, {f.base.base.loc, exp});
        args_PyUnicode_AsUTF8AndSize.push_back(al,
            {f.base.base.loc, ASRUtils::EXPR(ASR::make_PointerNullConstant_t(al, f.base.base.loc, ptr_t))});
        conv_result = ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc,
                            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                                sym_PyUnicode_AsUTF8AndSize, nullptr, args_PyUnicode_AsUTF8AndSize.p,
                                args_PyUnicode_AsUTF8AndSize.n, i1ptr_type, nullptr, nullptr)),
                            ASR::RealToReal, type, nullptr));
    } else {
        throw LCompilersException(
            "Returning from CPython with " + ASRUtils::get_type_code(type) + " type not supported");
    }

    LCOMPILERS_ASSERT(conv_result);
    return conv_result;
}

ASR::expr_t *native_to_cpython(Allocator &al, ASR::expr_t *exp, const ASR::Function_t &f,
                                const SymbolTable &parent_scope) {
    ASR::ttype_t *i4_type = ASRUtils::TYPE(ASR::make_Integer_t(al, f.base.base.loc, 4));
    ASR::ttype_t *i8_type = ASRUtils::TYPE(ASR::make_Integer_t(al, f.base.base.loc, 8));
    ASR::ttype_t *u8_type = ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al, f.base.base.loc, 8));
    ASR::ttype_t *f8_type = ASRUtils::TYPE(ASR::make_Real_t(al, f.base.base.loc, 8));
    ASR::ttype_t *ptr_t = ASRUtils::TYPE(ASR::make_CPtr_t(al, f.base.base.loc));

    ASR::expr_t *conv_result = nullptr;
    ASR::ttype_t *type = ASRUtils::expr_type(exp);
    if (type->type == ASR::ttypeType::Integer) {
        ASR::symbol_t *sym_PyLong_FromLongLong = parent_scope.get_symbol("PyLong_FromLongLong");
        Vec<ASR::call_arg_t> args_PyLong_FromLongLong;
        args_PyLong_FromLongLong.reserve(al, 1);
        args_PyLong_FromLongLong.push_back(al,
            {f.base.base.loc, ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc, exp,
                                ASR::cast_kindType::IntegerToInteger, i8_type, nullptr))});
        conv_result = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                        sym_PyLong_FromLongLong, nullptr, args_PyLong_FromLongLong.p, args_PyLong_FromLongLong.n,
                        ptr_t, nullptr, nullptr));
    } else if (type->type == ASR::ttypeType::UnsignedInteger) {
        ASR::symbol_t *sym_PyLong_FromUnsignedLongLong = parent_scope.get_symbol("PyLong_FromUnsignedLongLong");
        Vec<ASR::call_arg_t> args_PyLong_FromUnsignedLongLong;
        args_PyLong_FromUnsignedLongLong.reserve(al, 1);
        args_PyLong_FromUnsignedLongLong.push_back(al,
            {f.base.base.loc, ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc, exp,
                                ASR::cast_kindType::UnsignedIntegerToUnsignedInteger, u8_type, nullptr))});
        conv_result = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                        sym_PyLong_FromUnsignedLongLong, nullptr, args_PyLong_FromUnsignedLongLong.p,
                        args_PyLong_FromUnsignedLongLong.n, ptr_t, nullptr, nullptr));
    } else if (type->type == ASR::ttypeType::Logical) {
        ASR::symbol_t *sym_PyBool_FromLong = parent_scope.get_symbol("PyBool_FromLong");
        Vec<ASR::call_arg_t> args_PyBool_FromLong;
        args_PyBool_FromLong.reserve(al, 1);
        args_PyBool_FromLong.push_back(al,
            {f.base.base.loc, ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc, exp,
                                ASR::cast_kindType::LogicalToInteger, i4_type, nullptr))});
        conv_result = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyBool_FromLong,
                        nullptr, args_PyBool_FromLong.p, args_PyBool_FromLong.n, ptr_t, nullptr, nullptr));
    } else if (type->type == ASR::ttypeType::Real) {
        ASR::symbol_t *sym_PyFloat_FromDouble = parent_scope.get_symbol("PyFloat_FromDouble");
        Vec<ASR::call_arg_t> args_PyFloat_FromDouble;
        args_PyFloat_FromDouble.reserve(al, 1);
        args_PyFloat_FromDouble.push_back(al,
            {f.base.base.loc, ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc, exp,
                ASR::cast_kindType::RealToReal, f8_type, nullptr))});
        conv_result = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyFloat_FromDouble,
                        nullptr, args_PyFloat_FromDouble.p, args_PyFloat_FromDouble.n, ptr_t, nullptr, nullptr));
    } else if (type->type == ASR::ttypeType::Character) {
        ASR::symbol_t *sym_PyUnicode_FromString = parent_scope.get_symbol("PyUnicode_FromString");
        Vec<ASR::call_arg_t> args_PyUnicode_FromString;
        args_PyUnicode_FromString.reserve(al, 1);
        args_PyUnicode_FromString.push_back(al, {f.base.base.loc, exp});
        conv_result = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                        sym_PyUnicode_FromString, nullptr, args_PyUnicode_FromString.p, args_PyUnicode_FromString.n,
                        ptr_t, nullptr, nullptr));
    } else {
        throw LCompilersException(
            "Calling CPython with " + ASRUtils::get_type_code(ASRUtils::expr_type(exp)) + " type not supported");
    }

    LCOMPILERS_ASSERT(conv_result);
    return conv_result;
}

void generate_body(Allocator &al, ASR::Function_t &f, SymbolTable &parent_scope) {
    Vec<ASR::stmt_t*> body;
    body.reserve(al, 1);
    Str s;
    
    /*
        if (!Py_IsInitialized()) {
            Py_Initialize();
            int pS = PyRun_SimpleString("import sys; sys.path.append('.')");
        }
    */
    ASR::ttype_t *i4_type = ASRUtils::TYPE(ASR::make_Integer_t(al, f.base.base.loc, 4));

    ASR::symbol_t *sym_Py_IsInitialized = parent_scope.get_symbol("Py_IsInitialized");
    ASR::symbol_t *sym_Py_Initialize = parent_scope.get_symbol("Py_Initialize");
    LCOMPILERS_ASSERT(sym_Py_IsInitialized)
    ASR::asr_t *call_Py_IsInitialized = ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                                            sym_Py_IsInitialized, nullptr, nullptr, 0, i4_type, nullptr, nullptr);
    ASR::asr_t * if_cond = ASR::make_IntegerCompare_t(al, f.base.base.loc, ASRUtils::EXPR(call_Py_IsInitialized),
                                ASR::cmpopType::Eq, ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, 
                                                        f.base.base.loc, 0, i4_type)), i4_type, nullptr);
    Vec<ASR::stmt_t*> if_body;
    if_body.reserve(al, 2);
    ASR::asr_t *call_Py_Initialize = ASRUtils::make_SubroutineCall_t_util(al, f.base.base.loc, sym_Py_Initialize,
                                        nullptr, nullptr, 0, nullptr, nullptr, false, false);
    if_body.push_back(al, ASRUtils::STMT(call_Py_Initialize));

    ASR::symbol_t *sym_PyRun_SimpleString = parent_scope.get_symbol("PyRun_SimpleString");
    Vec<ASR::call_arg_t> args_PyRun_SimpleString;
    s.from_str(al, "import sys; sys.path.append(\".\")");
    args_PyRun_SimpleString.reserve(al, 1);
    ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, f.base.base.loc, 1, s.size(), nullptr));
    args_PyRun_SimpleString.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_StringConstant_t(al,
                                                                f.base.base.loc, s.c_str(al), str_type))});
    s.from_str(al, "pS");
    ASR::asr_t *pS = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                        ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, i4_type, nullptr,
                        ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false);
    ASR::expr_t *pS_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc, ASR::down_cast<ASR::symbol_t>(pS)));
    f.m_symtab->add_symbol(std::string("pS"), ASR::down_cast<ASR::symbol_t>(pS));
    if_body.push_back(al, ASRUtils::STMT(
        ASR::make_Assignment_t(al, f.base.base.loc, pS_ref, 
            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyRun_SimpleString, nullptr,
                                args_PyRun_SimpleString.p, args_PyRun_SimpleString.n, i4_type, nullptr, nullptr)),
                                nullptr)));

    body.push_back(al, ASRUtils::STMT(ASR::make_If_t(al, f.base.base.loc, ASRUtils::EXPR(if_cond), if_body.p, if_body.n,
                                             nullptr, 0)));

    /*
        void *pName = PyUnicode_FromString(module_name);
        void *pModule = PyImport_Import(pName);
        void *pFunc = PyObject_GetAttrString(pModule, func_name);
    */
    ASR::ttype_t *ptr_t = ASRUtils::TYPE(ASR::make_CPtr_t(al, f.base.base.loc));

    ASR::symbol_t *sym_PyUnicode_FromString = parent_scope.get_symbol("PyUnicode_FromString");
    Vec<ASR::call_arg_t> args_PyUnicode_FromString;
    s.from_str(al, f.m_module_file);
    args_PyUnicode_FromString.reserve(al, 1);
    str_type = ASRUtils::TYPE(ASR::make_Character_t(al, f.base.base.loc, 1, s.size(), nullptr));
    args_PyUnicode_FromString.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_StringConstant_t(al,
                                                                f.base.base.loc, s.c_str(al), str_type))});
    s.from_str(al, "pName");
    ASR::asr_t *pName = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                            ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t, nullptr,
                            ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false);
    ASR::expr_t *pName_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc, ASR::down_cast<ASR::symbol_t>(pName)));
    f.m_symtab->add_symbol(std::string("pName"), ASR::down_cast<ASR::symbol_t>(pName));
    body.push_back(al, ASRUtils::STMT(
        ASR::make_Assignment_t(al, f.base.base.loc, pName_ref, 
            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyUnicode_FromString, nullptr,
                            args_PyUnicode_FromString.p, args_PyUnicode_FromString.n, ptr_t, nullptr, nullptr)),
                            nullptr)));
    
    ASR::symbol_t *sym_PyImport_Import = parent_scope.get_symbol("PyImport_Import");
    Vec<ASR::call_arg_t> args_PyImport_Import;
    args_PyImport_Import.reserve(al, 1);
    args_PyImport_Import.push_back(al, {f.base.base.loc, pName_ref});
    s.from_str(al, "pModule");
    ASR::asr_t *pModule = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                            ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t, nullptr,
                            ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false);
    ASR::expr_t *pModule_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                    ASR::down_cast<ASR::symbol_t>(pModule)));
    f.m_symtab->add_symbol(std::string("pModule"), ASR::down_cast<ASR::symbol_t>(pModule));
    body.push_back(al, ASRUtils::STMT(
        ASR::make_Assignment_t(al, f.base.base.loc, pModule_ref, 
            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyImport_Import, nullptr,
                            args_PyImport_Import.p, args_PyImport_Import.n, ptr_t, nullptr, nullptr)),
                            nullptr)));
    
    ASR::symbol_t *sym_PyObject_GetAttrString = parent_scope.get_symbol("PyObject_GetAttrString");
    Vec<ASR::call_arg_t> args_PyObject_GetAttrString;
    s.from_str(al, f.m_module_file);
    args_PyObject_GetAttrString.reserve(al, 2);
    args_PyObject_GetAttrString.push_back(al, {f.base.base.loc, pModule_ref});
    s.from_str(al, f.m_name);
    str_type = ASRUtils::TYPE(ASR::make_Character_t(al, f.base.base.loc, 1, s.size(), nullptr));
    args_PyObject_GetAttrString.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_StringConstant_t(al,
                                                                    f.base.base.loc, s.c_str(al), str_type))});
    s.from_str(al, "pFunc");
    ASR::asr_t *pFunc = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                            ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t, nullptr,
                            ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false);
    ASR::expr_t *pFunc_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc, ASR::down_cast<ASR::symbol_t>(pFunc)));
    f.m_symtab->add_symbol(std::string("pFunc"), ASR::down_cast<ASR::symbol_t>(pFunc));
    body.push_back(al, ASRUtils::STMT(
        ASR::make_Assignment_t(al, f.base.base.loc, pFunc_ref, 
            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyObject_GetAttrString, nullptr,
                                        args_PyObject_GetAttrString.p, args_PyObject_GetAttrString.n, ptr_t, nullptr,
                                        nullptr)), nullptr)));

    // creating CPython tuple for arguments list
    ASR::symbol_t *sym_PyTuple_New = parent_scope.get_symbol("PyTuple_New");
    Vec<ASR::call_arg_t> args_PyTuple_New;
    args_PyTuple_New.reserve(al, 1);
    args_PyTuple_New.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, f.base.base.loc,
                                                        f.n_args, i4_type))});
    s.from_str(al, "pArgs");
    ASR::asr_t *pArgs = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                            ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t, nullptr,
                            ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false);
    ASR::expr_t *pArgs_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc, ASR::down_cast<ASR::symbol_t>(pArgs)));
    f.m_symtab->add_symbol(std::string("pArgs"), ASR::down_cast<ASR::symbol_t>(pArgs));
    body.push_back(al, ASRUtils::STMT(
        ASR::make_Assignment_t(al, f.base.base.loc, pArgs_ref, 
            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyTuple_New, nullptr,
                                args_PyTuple_New.p, args_PyTuple_New.n, ptr_t, nullptr, nullptr)), nullptr)));

    // Converting arguments to CPython types
    ASR::symbol_t *sym_PyTuple_SetItem = parent_scope.get_symbol("PyTuple_SetItem");
    for (size_t i = 0; i < f.n_args; i++) {
        Vec<ASR::call_arg_t> args_PyTuple_SetItem;
        args_PyTuple_SetItem.reserve(al, 3);
        args_PyTuple_SetItem.push_back(al, {f.base.base.loc, pArgs_ref});
        args_PyTuple_SetItem.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, 
                                                                f.base.base.loc, i, i4_type))});
        args_PyTuple_SetItem.push_back(al, {f.base.base.loc, native_to_cpython(al, f.m_args[i], f, parent_scope)});
        std::string p = "pA" + std::to_string(i);
        s.from_str(al, p);
        ASR::asr_t *pA = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                            ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, i4_type, nullptr,
                            ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pA_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc, ASR::down_cast<ASR::symbol_t>(pA)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pA));
        body.push_back(al,
            ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pA_ref,
                ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyTuple_SetItem, nullptr, 
                            args_PyTuple_SetItem.p, args_PyTuple_SetItem.n, i4_type, nullptr, nullptr)), nullptr)));
    }

    // calling CPython Function
    ASR::symbol_t *sym_PyObject_CallObject = parent_scope.get_symbol("PyObject_CallObject");
    Vec<ASR::call_arg_t> args_PyObject_CallObject;
    args_PyObject_CallObject.reserve(al, 2);
    args_PyObject_CallObject.push_back(al, {f.base.base.loc, pFunc_ref});
    args_PyObject_CallObject.push_back(al, {f.base.base.loc, pArgs_ref});
    s.from_str(al, "pReturn");
    ASR::asr_t *pReturn = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                            ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t, nullptr,
                            ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false);
    ASR::expr_t *pReturn_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                    ASR::down_cast<ASR::symbol_t>(pReturn)));
    f.m_symtab->add_symbol(std::string("pReturn"), ASR::down_cast<ASR::symbol_t>(pReturn));
    body.push_back(al, ASRUtils::STMT(
        ASR::make_Assignment_t(al, f.base.base.loc, pReturn_ref, 
            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyObject_CallObject, nullptr,
                                args_PyObject_CallObject.p, args_PyObject_CallObject.n, ptr_t, nullptr, nullptr)),
                            nullptr)));

    // Converting CPython result to native type
    ASR::ttype_t *ret_type = ASRUtils::get_FunctionType(f)->m_return_var_type;
    if (ret_type) {
        std::string return_var_name = "_lpython_return_variable";
        ASR::asr_t *ret_var = (ASR::asr_t*)f.m_symtab->get_symbol(return_var_name);
        LCOMPILERS_ASSERT(ret_var);
        ASR::expr_t *ret_var_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                        ASR::down_cast<ASR::symbol_t>(ret_var)));
        ASR::expr_t *ret_conv_result = cpython_to_native(al, pReturn_ref, ret_type, f, parent_scope);
        body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, ret_var_ref, ret_conv_result,
                                            nullptr)));
    }

    /*
        Py_DecRef(pName);
        Py_DecRef(pArgs);
        Py_DecRef(pReturn);
    */
    ASR::symbol_t *sym_Py_DecRef = parent_scope.get_symbol("Py_DecRef");

    Vec<ASR::call_arg_t> args_Py_DecRef;
    args_Py_DecRef.reserve(al, 1);                                
    args_Py_DecRef.push_back(al, {f.base.base.loc, pName_ref});
    body.push_back(al, ASRUtils::STMT(ASRUtils::make_SubroutineCall_t_util(al, f.base.base.loc, sym_Py_DecRef, nullptr,
                        args_Py_DecRef.p, args_Py_DecRef.n, nullptr, nullptr, false, false)));

    Vec<ASR::call_arg_t> args_Py_DecRef2;
    args_Py_DecRef2.reserve(al, 1);
    args_Py_DecRef2.push_back(al, {f.base.base.loc, pArgs_ref});
    body.push_back(al, ASRUtils::STMT(ASRUtils::make_SubroutineCall_t_util(al, f.base.base.loc, sym_Py_DecRef, nullptr,
                        args_Py_DecRef2.p, args_Py_DecRef2.n, nullptr, nullptr, false, false)));

    Vec<ASR::call_arg_t> args_Py_DecRef3;
    args_Py_DecRef3.reserve(al, 1);
    args_Py_DecRef3.push_back(al, {f.base.base.loc, pReturn_ref});
    body.push_back(al, ASRUtils::STMT(ASRUtils::make_SubroutineCall_t_util(al, f.base.base.loc, sym_Py_DecRef, nullptr,
                        args_Py_DecRef3.p, args_Py_DecRef3.n, nullptr, nullptr, false, false)));

    // reassignment
    f.m_body = body.p;
    f.n_body = body.n;
    ASRUtils::get_FunctionType(f)->m_abi = ASR::abiType::BindC;
    ASRUtils::get_FunctionType(f)->m_deftype = ASR::deftypeType::Implementation;
}

void pass_python_bind(Allocator &al, ASR::TranslationUnit_t &unit, const PassOptions &pass_options) {
    if (pass_options.c_backend) {
        // FIXME: C backend supports arrays, it is used in bindpy_02 to bindpy_04 tests.
        //  This pass currently does not handle any aggregate types.
        //  Once we include support for aggregate types, we should remove this check, and
        //  remove the `c_backend` variable from PassOptions.
        //  We will also need to remove the special handling of BindPython ABI in C
        //  backend as it would be handled by this ASR pass.
        return;
    }
    bool bindpython_used = false;
    for (auto &item : unit.m_symtab->get_scope()) {
        if (ASR::is_a<ASR::Function_t>(*item.second)) {
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(item.second);
            if (ASRUtils::get_FunctionType(f)->m_abi == ASR::abiType::BindPython) {
                if (f->n_body == 0 && f->m_module_file) {
                    bindpython_used = true;
                    generate_body(al, *f, *unit.m_symtab);
                }
            }
        }
        else if (ASR::is_a<ASR::Module_t>(*item.second)) {
            ASR::Module_t *module = ASR::down_cast<ASR::Module_t>(item.second);
            for (auto &module_item: module->m_symtab->get_scope() ) {
                if (ASR::is_a<ASR::Function_t>(*module_item.second)) {
                    ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(module_item.second);
                    if (ASRUtils::get_FunctionType(f)->m_abi == ASR::abiType::BindPython) {
                        if (f->n_body == 0 && f->m_module_file) {
                            bindpython_used = true;
                            generate_body(al, *f, *module->m_symtab);
                        }
                    }
                }
            }
        }

    }
    if (bindpython_used) { 
        PassUtils::UpdateDependenciesVisitor u(al);
        u.visit_TranslationUnit(unit);
    }
}
}
