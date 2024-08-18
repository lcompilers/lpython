#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/replace_arr_slice.h>
#include <libasr/pass/intrinsic_function_registry.h>


namespace LCompilers {

inline int get_random_number() {
    static int x = 0;
    return x++;
}

ASR::expr_t *cpython_to_native(Allocator &al, ASR::expr_t *exp, ASR::ttype_t *type, const ASR::Function_t &f,
                                Vec<ASR::stmt_t*> &body) {
    ASR::ttype_t *i1_type = ASRUtils::TYPE(ASR::make_Integer_t(al, f.base.base.loc, 1));
    ASR::ttype_t *i1ptr_type = ASRUtils::TYPE(ASR::make_Pointer_t(al, f.base.base.loc, i1_type));
    ASR::ttype_t *i4_type = ASRUtils::TYPE(ASR::make_Integer_t(al, f.base.base.loc, 4));
    ASR::ttype_t *i8_type = ASRUtils::TYPE(ASR::make_Integer_t(al, f.base.base.loc, 8));
    ASR::ttype_t *u8_type = ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al, f.base.base.loc, 8));
    ASR::ttype_t *f8_type = ASRUtils::TYPE(ASR::make_Real_t(al, f.base.base.loc, 8));
    ASR::ttype_t *ptr_t = ASRUtils::TYPE(ASR::make_CPtr_t(al, f.base.base.loc));

    ASR::expr_t *conv_result = nullptr;
    if (type->type == ASR::ttypeType::Integer) {
        ASR::symbol_t *sym_PyLong_AsLongLong = f.m_symtab->resolve_symbol("PyLong_AsLongLong");
        Vec<ASR::call_arg_t> args_PyLong_AsLongLong;
        args_PyLong_AsLongLong.reserve(al, 1);
        args_PyLong_AsLongLong.push_back(al, {f.base.base.loc, exp});
        conv_result = ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc,
                            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                                sym_PyLong_AsLongLong, nullptr, args_PyLong_AsLongLong.p, args_PyLong_AsLongLong.n,
                                i8_type, nullptr, nullptr)),
                            ASR::IntegerToInteger, type, nullptr));
    } else if (type->type == ASR::ttypeType::UnsignedInteger) {
        ASR::symbol_t *sym_PyLong_AsUnsignedLongLong = f.m_symtab->resolve_symbol("PyLong_AsUnsignedLongLong");
        Vec<ASR::call_arg_t> args_PyLong_AsUnsignedLongLong;
        args_PyLong_AsUnsignedLongLong.reserve(al, 1);
        args_PyLong_AsUnsignedLongLong.push_back(al, {f.base.base.loc, exp});
        conv_result = ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc, 
                            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                                sym_PyLong_AsUnsignedLongLong, nullptr, args_PyLong_AsUnsignedLongLong.p,
                                args_PyLong_AsUnsignedLongLong.n, u8_type, nullptr, nullptr)),
                            ASR::UnsignedIntegerToUnsignedInteger, type, nullptr));
    } else if (type->type == ASR::ttypeType::Real) {
        ASR::symbol_t *sym_PyFloat_AsDouble = f.m_symtab->resolve_symbol("PyFloat_AsDouble");
        Vec<ASR::call_arg_t> args_PyFloat_AsDouble;
        args_PyFloat_AsDouble.reserve(al, 1);
        args_PyFloat_AsDouble.push_back(al, {f.base.base.loc, exp});
        conv_result = ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc,
                            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                                sym_PyFloat_AsDouble, nullptr, args_PyFloat_AsDouble.p, args_PyFloat_AsDouble.n,
                                f8_type, nullptr, nullptr)),
                            ASR::RealToReal, type, nullptr));
    } else if (type->type == ASR::ttypeType::Logical) {
        ASR::symbol_t *sym_PyObject_IsTrue = f.m_symtab->resolve_symbol("PyObject_IsTrue");
        Vec<ASR::call_arg_t> args_PyObject_IsTrue;
        args_PyObject_IsTrue.reserve(al, 1);
        args_PyObject_IsTrue.push_back(al, {f.base.base.loc, exp});
        conv_result = ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc, 
                            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                                sym_PyObject_IsTrue, nullptr, args_PyObject_IsTrue.p, args_PyObject_IsTrue.n,
                                i4_type, nullptr, nullptr)),
                            ASR::IntegerToLogical, type, nullptr));
    } else if (type->type == ASR::ttypeType::Character) {
        ASR::symbol_t *sym_PyUnicode_AsUTF8AndSize = f.m_symtab->resolve_symbol("PyUnicode_AsUTF8AndSize");
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
    
    } else if (type->type == ASR::ttypeType::List) {
        ASR::List_t *list = ASR::down_cast<ASR::List_t>(type);
        Str s;

        ASR::symbol_t *sym_PyList_Size = f.m_symtab->resolve_symbol("PyList_Size");
        Vec<ASR::call_arg_t> args_PyList_Size;
        args_PyList_Size.reserve(al, 1);
        args_PyList_Size.push_back(al, {f.base.base.loc, exp});
        std::string p = "_size" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pSize = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, i8_type,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pSize_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                                        ASR::down_cast<ASR::symbol_t>(pSize)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pSize));
        body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pSize_ref,
                            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                                sym_PyList_Size, nullptr, args_PyList_Size.p,
                                args_PyList_Size.n, i8_type, nullptr, nullptr)), nullptr)));
        
        p = "_i" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pI = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, i8_type,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pI_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                                        ASR::down_cast<ASR::symbol_t>(pI)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pI));
        body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pI_ref,
                            ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, f.base.base.loc, 0, i8_type)), nullptr)));
        
        p = "_result" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pResult = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, type,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pResult_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                                        ASR::down_cast<ASR::symbol_t>(pResult)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pResult));
        body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pResult_ref,
                            ASRUtils::EXPR(ASR::make_ListConstant_t(al, f.base.base.loc, nullptr, 0, type)), nullptr)));

        Vec<ASR::stmt_t*> while_body;
        while_body.reserve(al, 2);
        
        ASR::symbol_t *sym_PyList_GetItem = f.m_symtab->resolve_symbol("PyList_GetItem");
        Vec<ASR::call_arg_t> args_PyList_GetItem;
        args_PyList_GetItem.reserve(al, 2);
        args_PyList_GetItem.push_back(al, {f.base.base.loc, exp});
        args_PyList_GetItem.push_back(al, {f.base.base.loc, pI_ref});

        while_body.push_back(al, ASRUtils::STMT(ASR::make_ListAppend_t(al, f.base.base.loc, pResult_ref, 
                cpython_to_native(al,  ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                                sym_PyList_GetItem, nullptr, args_PyList_GetItem.p,
                                args_PyList_GetItem.n, ptr_t, nullptr, nullptr)), list->m_type, f, while_body)
            )));

        while_body.push_back(al, ASRUtils::STMT(
            ASR::make_Assignment_t(al, f.base.base.loc,
                pI_ref,
                ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, f.base.base.loc, pI_ref, ASR::binopType::Add,
                    ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, f.base.base.loc, 1, i8_type)), i8_type, nullptr)),
            nullptr)));

        body.push_back(al, ASRUtils::STMT(
            ASR::make_WhileLoop_t(al, f.base.base.loc, nullptr,
                ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, f.base.base.loc, pI_ref, ASR::cmpopType::Lt, pSize_ref,
                    i1_type, nullptr)),
                while_body.p, while_body.n, nullptr, 0)));
        
        conv_result = pResult_ref;

    } else if (type->type == ASR::ttypeType::Tuple) {
        ASR::Tuple_t *tuple = ASR::down_cast<ASR::Tuple_t>(type);
        Str s;

        std::string p = "_result" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pResult = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, type,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pResult_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                                        ASR::down_cast<ASR::symbol_t>(pResult)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pResult));

        ASR::symbol_t *sym_PyTuple_GetItem = f.m_symtab->resolve_symbol("PyTuple_GetItem");
        Vec<ASR::expr_t*> tuple_elements;
        tuple_elements.reserve(al, tuple->n_type);
        for (size_t i = 0; i < tuple->n_type; i++) {
            Vec<ASR::call_arg_t> args_PyTuple_GetItem;
            args_PyTuple_GetItem.reserve(al, 2);
            args_PyTuple_GetItem.push_back(al, {f.base.base.loc, exp});
            args_PyTuple_GetItem.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_IntegerConstant_t(al,
                                                                                    f.base.base.loc, i, i8_type))});
            tuple_elements.push_back(al, cpython_to_native(al,  ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al,
                                                                    f.base.base.loc, sym_PyTuple_GetItem, nullptr,
                                                                    args_PyTuple_GetItem.p, args_PyTuple_GetItem.n,
                                                                    ptr_t, nullptr, nullptr)),
                                                                tuple->m_type[i], f, body));
        }

        body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pResult_ref,
            ASRUtils::EXPR(ASR::make_TupleConstant_t(al, f.base.base.loc, tuple_elements.p, tuple_elements.n, type)),
            nullptr)));
        conv_result = pResult_ref;

    } else if (type->type == ASR::ttypeType::Set) {
        ASR::Set_t *set = ASR::down_cast<ASR::Set_t>(type);
        Str s;

        std::string p = "_result" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pResult = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, type,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pResult_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                                        ASR::down_cast<ASR::symbol_t>(pResult)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pResult));

        body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pResult_ref,
            ASRUtils::EXPR(ASR::make_SetConstant_t(al, f.base.base.loc, nullptr, 0, type)),
            nullptr)));
        
        ASR::symbol_t *sym_PySet_Size = f.m_symtab->resolve_symbol("PySet_Size");
        Vec<ASR::call_arg_t> args_PySet_Size;
        args_PySet_Size.reserve(al, 1);
        args_PySet_Size.push_back(al, {f.base.base.loc, exp});
        p = "_size" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pSize = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, i8_type,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pSize_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                                        ASR::down_cast<ASR::symbol_t>(pSize)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pSize));
        body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pSize_ref,
                            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                                sym_PySet_Size, nullptr, args_PySet_Size.p,
                                args_PySet_Size.n, i8_type, nullptr, nullptr)), nullptr)));

        p = "_i" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pI = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, i8_type,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pI_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                                        ASR::down_cast<ASR::symbol_t>(pI)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pI));
        body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pI_ref,
                            ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, f.base.base.loc, 0, i8_type)), nullptr)));

        p = "_iterator" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pIterator = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pIterator_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                                        ASR::down_cast<ASR::symbol_t>(pIterator)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pIterator));

        ASR::symbol_t *sym_PyObject_GetIter = f.m_symtab->resolve_symbol("PyObject_GetIter"); // TODO: decrement
        Vec<ASR::call_arg_t> args_PyObject_GetIter;
        args_PyObject_GetIter.reserve(al, 1);
        args_PyObject_GetIter.push_back(al, {f.base.base.loc, exp});
        body.push_back(al, 
            ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pIterator_ref,
                ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyObject_GetIter, nullptr,
                    args_PyObject_GetIter.p, args_PyObject_GetIter.n, ptr_t, nullptr, nullptr)), nullptr)));

        p = "_i" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pItem = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pItem_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                                        ASR::down_cast<ASR::symbol_t>(pItem)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pItem));
        
        Vec<ASR::stmt_t*> while_body;
        while_body.reserve(al, 3);

        while_body.push_back(al, ASRUtils::STMT(
            ASR::make_Assignment_t(al, f.base.base.loc,
                pI_ref,
                ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, f.base.base.loc, pI_ref, ASR::binopType::Add,
                    ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, f.base.base.loc, 1, i8_type)), i8_type, nullptr)),
            nullptr)));
        
        ASR::symbol_t *sym_PyIter_Next = f.m_symtab->resolve_symbol("PyIter_Next"); // TODO: decrement
        Vec<ASR::call_arg_t> args_PyIter_Next;
        args_PyIter_Next.reserve(al, 1);
        args_PyIter_Next.push_back(al, {f.base.base.loc, pIterator_ref});

        body.push_back(al,
            ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pItem_ref,
                ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyIter_Next, nullptr,
                    args_PyIter_Next.p, args_PyIter_Next.n, ptr_t, nullptr, nullptr)), nullptr)));

        Vec<ASR::expr_t*> args_Set_add;
        args_Set_add.reserve(al, 2);
        args_Set_add.push_back(al, pResult_ref);
        args_Set_add.push_back(al, cpython_to_native(al, pItem_ref, set->m_type, f, while_body));

        while_body.push_back(al, ASRUtils::STMT(ASR::make_Expr_t(al, f.base.base.loc,
            ASRUtils::EXPR(ASR::make_IntrinsicElementalFunction_t(al, f.base.base.loc,
            static_cast<int64_t>(ASRUtils::IntrinsicElementalFunctions::SetAdd),
            args_Set_add.p, args_Set_add.n, 0, nullptr, nullptr)))));

        while_body.push_back(al,
            ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pItem_ref,
                ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyIter_Next, nullptr,
                    args_PyIter_Next.p, args_PyIter_Next.n, ptr_t, nullptr, nullptr)), nullptr)));

        body.push_back(al, ASRUtils::STMT(ASR::make_WhileLoop_t(al, f.base.base.loc, nullptr, 
            ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, f.base.base.loc, pI_ref, ASR::cmpopType::Lt, pSize_ref,
                    i1_type, nullptr)),
            while_body.p, while_body.n, nullptr, 0)));

        conv_result = pResult_ref;

    } else {
        throw LCompilersException(
            "Returning from CPython with " + ASRUtils::get_type_code(type) + " type not supported");
    }

    LCOMPILERS_ASSERT(conv_result);
    return conv_result;
}

ASR::expr_t *native_to_cpython(Allocator &al, ASR::expr_t *exp, const ASR::Function_t &f, Vec<ASR::stmt_t*> &body) {
    ASR::ttype_t *i1_type = ASRUtils::TYPE(ASR::make_Logical_t(al, f.base.base.loc, 1));
    ASR::ttype_t *i4_type = ASRUtils::TYPE(ASR::make_Integer_t(al, f.base.base.loc, 4));
    ASR::ttype_t *i8_type = ASRUtils::TYPE(ASR::make_Integer_t(al, f.base.base.loc, 8));
    ASR::ttype_t *u8_type = ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al, f.base.base.loc, 8));
    ASR::ttype_t *f8_type = ASRUtils::TYPE(ASR::make_Real_t(al, f.base.base.loc, 8));
    ASR::ttype_t *ptr_t = ASRUtils::TYPE(ASR::make_CPtr_t(al, f.base.base.loc));

    ASR::expr_t *conv_result = nullptr;
    ASR::ttype_t *type = ASRUtils::expr_type(exp);
    if (type->type == ASR::ttypeType::Integer) {
        ASR::symbol_t *sym_PyLong_FromLongLong = f.m_symtab->resolve_symbol("PyLong_FromLongLong");
        Vec<ASR::call_arg_t> args_PyLong_FromLongLong;
        args_PyLong_FromLongLong.reserve(al, 1);
        args_PyLong_FromLongLong.push_back(al,
            {f.base.base.loc, ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc, exp,
                                ASR::cast_kindType::IntegerToInteger, i8_type, nullptr))});
        conv_result = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                        sym_PyLong_FromLongLong, nullptr, args_PyLong_FromLongLong.p, args_PyLong_FromLongLong.n,
                        ptr_t, nullptr, nullptr));
    } else if (type->type == ASR::ttypeType::UnsignedInteger) {
        ASR::symbol_t *sym_PyLong_FromUnsignedLongLong = f.m_symtab->resolve_symbol("PyLong_FromUnsignedLongLong");
        Vec<ASR::call_arg_t> args_PyLong_FromUnsignedLongLong;
        args_PyLong_FromUnsignedLongLong.reserve(al, 1);
        args_PyLong_FromUnsignedLongLong.push_back(al,
            {f.base.base.loc, ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc, exp,
                                ASR::cast_kindType::UnsignedIntegerToUnsignedInteger, u8_type, nullptr))});
        conv_result = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                        sym_PyLong_FromUnsignedLongLong, nullptr, args_PyLong_FromUnsignedLongLong.p,
                        args_PyLong_FromUnsignedLongLong.n, ptr_t, nullptr, nullptr));
    } else if (type->type == ASR::ttypeType::Logical) {
        ASR::symbol_t *sym_PyBool_FromLong = f.m_symtab->resolve_symbol("PyBool_FromLong");
        Vec<ASR::call_arg_t> args_PyBool_FromLong;
        args_PyBool_FromLong.reserve(al, 1);
        args_PyBool_FromLong.push_back(al,
            {f.base.base.loc, ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc, exp,
                                ASR::cast_kindType::LogicalToInteger, i4_type, nullptr))});
        conv_result = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyBool_FromLong,
                        nullptr, args_PyBool_FromLong.p, args_PyBool_FromLong.n, ptr_t, nullptr, nullptr));
    } else if (type->type == ASR::ttypeType::Real) {
        ASR::symbol_t *sym_PyFloat_FromDouble = f.m_symtab->resolve_symbol("PyFloat_FromDouble");
        Vec<ASR::call_arg_t> args_PyFloat_FromDouble;
        args_PyFloat_FromDouble.reserve(al, 1);
        args_PyFloat_FromDouble.push_back(al,
            {f.base.base.loc, ASRUtils::EXPR(ASR::make_Cast_t(al, f.base.base.loc, exp,
                ASR::cast_kindType::RealToReal, f8_type, nullptr))});
        conv_result = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyFloat_FromDouble,
                        nullptr, args_PyFloat_FromDouble.p, args_PyFloat_FromDouble.n, ptr_t, nullptr, nullptr));
    } else if (type->type == ASR::ttypeType::Character) {
        ASR::symbol_t *sym_PyUnicode_FromString = f.m_symtab->resolve_symbol("PyUnicode_FromString");
        Vec<ASR::call_arg_t> args_PyUnicode_FromString;
        args_PyUnicode_FromString.reserve(al, 1);
        args_PyUnicode_FromString.push_back(al, {f.base.base.loc, exp});
        conv_result = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc,
                        sym_PyUnicode_FromString, nullptr, args_PyUnicode_FromString.p, args_PyUnicode_FromString.n,
                        ptr_t, nullptr, nullptr));
    } else if (type->type == ASR::ttypeType::Tuple) {
        ASR::Tuple_t *tuple = ASR::down_cast<ASR::Tuple_t>(type);
        Str s;

        ASR::symbol_t *sym_PyTuple_New = f.m_symtab->resolve_symbol("PyTuple_New");
        Vec<ASR::call_arg_t> args_PyTuple_New;
        args_PyTuple_New.reserve(al, 1);
        args_PyTuple_New.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, f.base.base.loc,
                                                            tuple->n_type, i4_type))});
        std::string p = "_" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pArgs = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pArgs_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                                    ASR::down_cast<ASR::symbol_t>(pArgs)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pArgs));
        body.push_back(al, ASRUtils::STMT(
            ASR::make_Assignment_t(al, f.base.base.loc, pArgs_ref, 
                ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyTuple_New, nullptr,
                                    args_PyTuple_New.p, args_PyTuple_New.n, ptr_t, nullptr, nullptr)), nullptr)));
        conv_result = pArgs_ref;

        ASR::symbol_t *sym_PyTuple_SetItem = f.m_symtab->resolve_symbol("PyTuple_SetItem");
        for (size_t i = 0; i < tuple->n_type; i++) {
            Vec<ASR::call_arg_t> args_PyTuple_SetItem;
            args_PyTuple_SetItem.reserve(al, 3);
            args_PyTuple_SetItem.push_back(al, {f.base.base.loc, pArgs_ref});
            ASR::expr_t *n = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, f.base.base.loc, i, i4_type));
            args_PyTuple_SetItem.push_back(al, {f.base.base.loc, n});
            args_PyTuple_SetItem.push_back(al, {f.base.base.loc, native_to_cpython(al, 
                                                                    ASRUtils::EXPR(ASR::make_TupleItem_t(al,
                                                                        f.base.base.loc, exp, n, tuple->m_type[i],
                                                                        nullptr)),
                                                                    f, body)});
            std::string p = "_" + std::to_string(get_random_number());
            s.from_str(al, p);
            ASR::asr_t *pA = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, i4_type,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
            ASR::expr_t *pA_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                                    ASR::down_cast<ASR::symbol_t>(pA)));
            f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pA));
            body.push_back(al,
                ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pA_ref,
                    ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyTuple_SetItem, nullptr,
                                args_PyTuple_SetItem.p, args_PyTuple_SetItem.n, i4_type, nullptr, nullptr)), nullptr)));
        }
    } else if (type->type == ASR::ttypeType::List) {
        ASR::List_t *list = ASR::down_cast<ASR::List_t>(type);
        Str s;

        ASR::symbol_t *sym_PyList_New = f.m_symtab->resolve_symbol("PyList_New");
        Vec<ASR::call_arg_t> args_PyList_New;
        args_PyList_New.reserve(al, 1);
        args_PyList_New.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, f.base.base.loc,
                                                            0, i8_type))});
        std::string p = "_" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pArgs = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pArgs_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                    ASR::down_cast<ASR::symbol_t>(pArgs)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pArgs));
        body.push_back(al, ASRUtils::STMT(
            ASR::make_Assignment_t(al, f.base.base.loc, pArgs_ref, 
                ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyList_New, nullptr,
                                    args_PyList_New.p, args_PyList_New.n, ptr_t, nullptr, nullptr)), nullptr)));
        conv_result = pArgs_ref;

        p = "_size" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pSize = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, i4_type,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pSize_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                                        ASR::down_cast<ASR::symbol_t>(pSize)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pSize));
        body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pSize_ref,
                            ASRUtils::EXPR(ASR::make_ListLen_t(al, f.base.base.loc, exp, i4_type, nullptr)), nullptr)));

        p = "_i" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pI = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, i4_type,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pI_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                                        ASR::down_cast<ASR::symbol_t>(pI)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pI));
        body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pI_ref,
                            ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, f.base.base.loc, 0, i4_type)), nullptr)));

        ASR::symbol_t *sym_PyList_Append = f.m_symtab->resolve_symbol("PyList_Append");
        p = "_item" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pItem = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, list->m_type,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pItem_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                                        ASR::down_cast<ASR::symbol_t>(pItem)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pItem));
        
        Vec<ASR::stmt_t*> while_body;
        while_body.reserve(al, 3);
        
        while_body.push_back(al, ASRUtils::STMT(
            ASR::make_Assignment_t(al, f.base.base.loc, 
                pItem_ref,
                ASRUtils::EXPR(ASR::make_ListItem_t(al, f.base.base.loc, exp, pI_ref, type, nullptr)),
            nullptr)));
        
        while_body.push_back(al, ASRUtils::STMT(
            ASR::make_Assignment_t(al, f.base.base.loc,
                pI_ref,
                ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, f.base.base.loc, pI_ref, ASR::binopType::Add,
                    ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, f.base.base.loc, 1, i4_type)), i4_type, nullptr)),
            nullptr)));

        Vec<ASR::call_arg_t> args_PyList_Append;
        args_PyList_Append.reserve(al, 2);
        args_PyList_Append.push_back(al, {f.base.base.loc, pArgs_ref});
        args_PyList_Append.push_back(al, {f.base.base.loc, native_to_cpython(al, pItem_ref, f, while_body)}); // TODO: decrement the reference count of the return value of native_to_cpython after the PyList_Append subroutine call in next line

        while_body.push_back(al, ASRUtils::STMT((ASRUtils::make_SubroutineCall_t_util(al, f.base.base.loc,
                                                    sym_PyList_Append, nullptr, args_PyList_Append.p,
                                                    args_PyList_Append.n, nullptr, nullptr, false, false))));

        body.push_back(al, ASRUtils::STMT(
            ASR::make_WhileLoop_t(al, f.base.base.loc, nullptr,
                ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, f.base.base.loc, pI_ref, ASR::cmpopType::Lt, pSize_ref,
                    i1_type, nullptr)),
                while_body.p, while_body.n, nullptr, 0)));

    } else if (type->type == ASR::ttypeType::Set) {
        ASR::Set_t *set = ASR::down_cast<ASR::Set_t>(type);
        Str s;

        ASR::symbol_t *sym_PySet_New = f.m_symtab->resolve_symbol("PySet_New");
        Vec<ASR::call_arg_t> args_PySet_New;
        args_PySet_New.reserve(al, 1);
        args_PySet_New.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_PointerNullConstant_t(al,
                                                                                f.base.base.loc, ptr_t))});
        std::string p = "_" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pArgs = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pArgs_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                    ASR::down_cast<ASR::symbol_t>(pArgs)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pArgs));
        body.push_back(al, ASRUtils::STMT(
            ASR::make_Assignment_t(al, f.base.base.loc, pArgs_ref, 
                ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PySet_New, nullptr,
                                    args_PySet_New.p, args_PySet_New.n, ptr_t, nullptr, nullptr)), nullptr)));
        conv_result = pArgs_ref;

        p = "_" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pItem = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, set->m_type,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pItem_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                    ASR::down_cast<ASR::symbol_t>(pItem)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pItem));

        Vec<ASR::stmt_t*> for_body;
        for_body.reserve(al, 1);
        ASR::symbol_t *sym_PySet_Add = f.m_symtab->resolve_symbol("PySet_Add");
        Vec<ASR::call_arg_t> args_PySet_Add;
        args_PySet_Add.reserve(al, 2);
        args_PySet_Add.push_back(al, {f.base.base.loc, pArgs_ref});
        args_PySet_Add.push_back(al, {f.base.base.loc, native_to_cpython(al, pItem_ref, f, for_body)}); // TODO: decrement the reference count of the return value of native_to_cpython after the PyList_Append subroutine call in next line
        for_body.push_back(al, ASRUtils::STMT((ASRUtils::make_SubroutineCall_t_util(al, f.base.base.loc,
                                                    sym_PySet_Add, nullptr, args_PySet_Add.p,
                                                    args_PySet_Add.n, nullptr, nullptr, false, false))));

        body.push_back(al, ASRUtils::STMT(ASR::make_ForEach_t(al, f.base.base.loc, pItem_ref, exp, for_body.p, for_body.n)));
    
    } else if (type->type == ASR::ttypeType::Dict) {
        ASR::Dict_t *dict = ASR::down_cast<ASR::Dict_t>(type);
        Str s;

        ASR::symbol_t *sym_PyDict_New = f.m_symtab->resolve_symbol("PyDict_New"); // TODO: decrement
        std::string p = "_" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pArgs = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pArgs_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                    ASR::down_cast<ASR::symbol_t>(pArgs)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pArgs));
        body.push_back(al, ASRUtils::STMT(
            ASR::make_Assignment_t(al, f.base.base.loc, pArgs_ref, 
                ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyDict_New, nullptr,
                                    nullptr, 0, ptr_t, nullptr, nullptr)), nullptr)));
        conv_result = pArgs_ref;

        p = "_" + std::to_string(get_random_number());
        s.from_str(al, p);
        ASR::asr_t *pItem = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                                ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, dict->m_key_type,
                                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pItem_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                    ASR::down_cast<ASR::symbol_t>(pItem)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pItem));

        Vec<ASR::stmt_t*> for_body;
        for_body.reserve(al, 1);
        ASR::symbol_t *sym_PyDict_SetItem = f.m_symtab->resolve_symbol("PyDict_SetItem");
        Vec<ASR::call_arg_t> args_PyDict_SetItem;
        args_PyDict_SetItem.reserve(al, 3);
        args_PyDict_SetItem.push_back(al, {f.base.base.loc, pArgs_ref});
        args_PyDict_SetItem.push_back(al, {f.base.base.loc, native_to_cpython(al, pItem_ref, f, for_body)}); // TODO: decrement the reference count of the return value of native_to_cpython after the PyList_Append subroutine call in next line
        args_PyDict_SetItem.push_back(al, {f.base.base.loc, native_to_cpython(al, 
            ASRUtils::EXPR(ASR::make_DictItem_t(al, f.base.base.loc, exp, pItem_ref, nullptr, dict->m_value_type, nullptr))
        , f, for_body)}); // TODO: decrement the reference count of the return value of native_to_cpython after the PyList_Append subroutine call in next line
        for_body.push_back(al, ASRUtils::STMT((ASRUtils::make_SubroutineCall_t_util(al, f.base.base.loc,
                                                    sym_PyDict_SetItem, nullptr, args_PyDict_SetItem.p,
                                                    args_PyDict_SetItem.n, nullptr, nullptr, false, false))));

        body.push_back(al, ASRUtils::STMT(ASR::make_ForEach_t(al, f.base.base.loc, pItem_ref, exp, for_body.p, for_body.n)));

    } else {
        throw LCompilersException(
            "Calling CPython with " + ASRUtils::get_type_code(ASRUtils::expr_type(exp)) + " type not supported");
    }

    LCOMPILERS_ASSERT(conv_result);
    return conv_result;
}

void generate_body(Allocator &al, ASR::Function_t &f) {
    Vec<ASR::stmt_t*> body;
    body.reserve(al, 1);
    Str s;

    /*
        if (!Py_IsInitialized()) {
            Py_Initialize();
            void *pA = Py_DecodeLocale("", NULL);
            PySys_SetArgv(1, &pA);
        }
    */
    ASR::ttype_t *i4_type = ASRUtils::TYPE(ASR::make_Integer_t(al, f.base.base.loc, 4));
    ASR::ttype_t *ptr_t = ASRUtils::TYPE(ASR::make_CPtr_t(al, f.base.base.loc));

    ASR::symbol_t *sym_Py_IsInitialized = f.m_symtab->resolve_symbol("Py_IsInitialized");
    ASR::symbol_t *sym_Py_Initialize = f.m_symtab->resolve_symbol("Py_Initialize");
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

    ASR::symbol_t *sym_Py_DecodeLocale = f.m_symtab->resolve_symbol("Py_DecodeLocale");
    Vec<ASR::call_arg_t> args_Py_DecodeLocale;
    s.from_str(al, "");
    args_Py_DecodeLocale.reserve(al, 1);
    ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, f.base.base.loc, 1, s.size(), nullptr));
    args_Py_DecodeLocale.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_StringConstant_t(al,
                                                                f.base.base.loc, s.c_str(al), str_type))});
    args_Py_DecodeLocale.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_PointerNullConstant_t(al,
                                                                                f.base.base.loc, ptr_t))});
    s.from_str(al, "pA");
    ASR::asr_t *pA = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                        ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t, nullptr,
                        ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
    ASR::expr_t *pA_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc, ASR::down_cast<ASR::symbol_t>(pA)));
    f.m_symtab->add_symbol(std::string("pA"), ASR::down_cast<ASR::symbol_t>(pA));
    if_body.push_back(al, ASRUtils::STMT(
        ASR::make_Assignment_t(al, f.base.base.loc, pA_ref,
            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_Py_DecodeLocale, nullptr,
                                args_Py_DecodeLocale.p, args_Py_DecodeLocale.n, ptr_t, nullptr, nullptr)),
                                nullptr)));
    
    ASR::symbol_t *sym_PySys_SetArgv = f.m_symtab->resolve_symbol("PySys_SetArgv");
    Vec<ASR::call_arg_t> args_PySys_SetArgv;
    s.from_str(al, "");
    args_PySys_SetArgv.reserve(al, 1);
    args_PySys_SetArgv.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_IntegerConstant_t(al,  f.base.base.loc,
                                                                                1, i4_type))});
    args_PySys_SetArgv.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_GetPointer_t(al, f.base.base.loc,
                                                                        pA_ref, ptr_t, nullptr))});
    if_body.push_back(al, ASRUtils::STMT((ASRUtils::make_SubroutineCall_t_util(al, f.base.base.loc, sym_PySys_SetArgv,
                                                nullptr, args_PySys_SetArgv.p, args_PySys_SetArgv.n, nullptr, nullptr,
                                                false, false))));

    body.push_back(al, ASRUtils::STMT(ASR::make_If_t(al, f.base.base.loc, ASRUtils::EXPR(if_cond), if_body.p, if_body.n,
                                             nullptr, 0)));

    /*
        void *pName = PyUnicode_FromString(module_name);
        void *pModule = PyImport_Import(pName);
        void *pFunc = PyObject_GetAttrString(pModule, func_name);
    */

    ASR::symbol_t *sym_PyUnicode_FromString = f.m_symtab->resolve_symbol("PyUnicode_FromString");
    Vec<ASR::call_arg_t> args_PyUnicode_FromString;
    s.from_str(al, f.m_module_file);
    args_PyUnicode_FromString.reserve(al, 1);
    str_type = ASRUtils::TYPE(ASR::make_Character_t(al, f.base.base.loc, 1, s.size(), nullptr));
    args_PyUnicode_FromString.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_StringConstant_t(al,
                                                                f.base.base.loc, s.c_str(al), str_type))});
    s.from_str(al, "pName");
    ASR::asr_t *pName = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                            ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t, nullptr,
                            ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
    ASR::expr_t *pName_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc, ASR::down_cast<ASR::symbol_t>(pName)));
    f.m_symtab->add_symbol(std::string("pName"), ASR::down_cast<ASR::symbol_t>(pName));
    body.push_back(al, ASRUtils::STMT(
        ASR::make_Assignment_t(al, f.base.base.loc, pName_ref,
            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyUnicode_FromString, nullptr,
                            args_PyUnicode_FromString.p, args_PyUnicode_FromString.n, ptr_t, nullptr, nullptr)),
                            nullptr)));

    ASR::symbol_t *sym_PyImport_Import = f.m_symtab->resolve_symbol("PyImport_Import");
    Vec<ASR::call_arg_t> args_PyImport_Import;
    args_PyImport_Import.reserve(al, 1);
    args_PyImport_Import.push_back(al, {f.base.base.loc, pName_ref});
    s.from_str(al, "pModule");
    ASR::asr_t *pModule = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                            ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t, nullptr,
                            ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
    ASR::expr_t *pModule_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc,
                                    ASR::down_cast<ASR::symbol_t>(pModule)));
    f.m_symtab->add_symbol(std::string("pModule"), ASR::down_cast<ASR::symbol_t>(pModule));
    body.push_back(al, ASRUtils::STMT(
        ASR::make_Assignment_t(al, f.base.base.loc, pModule_ref, 
            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyImport_Import, nullptr,
                            args_PyImport_Import.p, args_PyImport_Import.n, ptr_t, nullptr, nullptr)),
                            nullptr)));

    ASR::symbol_t *sym_PyObject_GetAttrString = f.m_symtab->resolve_symbol("PyObject_GetAttrString");
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
                            ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
    ASR::expr_t *pFunc_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc, ASR::down_cast<ASR::symbol_t>(pFunc)));
    f.m_symtab->add_symbol(std::string("pFunc"), ASR::down_cast<ASR::symbol_t>(pFunc));
    body.push_back(al, ASRUtils::STMT(
        ASR::make_Assignment_t(al, f.base.base.loc, pFunc_ref, 
            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyObject_GetAttrString, nullptr,
                                        args_PyObject_GetAttrString.p, args_PyObject_GetAttrString.n, ptr_t, nullptr,
                                        nullptr)), nullptr)));

    // creating CPython tuple for arguments list
    ASR::symbol_t *sym_PyTuple_New = f.m_symtab->resolve_symbol("PyTuple_New");
    Vec<ASR::call_arg_t> args_PyTuple_New;
    args_PyTuple_New.reserve(al, 1);
    args_PyTuple_New.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, f.base.base.loc,
                                                        f.n_args, i4_type))});
    s.from_str(al, "pArgs");
    ASR::asr_t *pArgs = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                            ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t, nullptr,
                            ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
    ASR::expr_t *pArgs_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc, ASR::down_cast<ASR::symbol_t>(pArgs)));
    f.m_symtab->add_symbol(std::string("pArgs"), ASR::down_cast<ASR::symbol_t>(pArgs));
    body.push_back(al, ASRUtils::STMT(
        ASR::make_Assignment_t(al, f.base.base.loc, pArgs_ref, 
            ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyTuple_New, nullptr,
                                args_PyTuple_New.p, args_PyTuple_New.n, ptr_t, nullptr, nullptr)), nullptr)));

    // Converting arguments to CPython types
    ASR::symbol_t *sym_PyTuple_SetItem = f.m_symtab->resolve_symbol("PyTuple_SetItem");
    for (size_t i = 0; i < f.n_args; i++) {
        Vec<ASR::call_arg_t> args_PyTuple_SetItem;
        args_PyTuple_SetItem.reserve(al, 3);
        args_PyTuple_SetItem.push_back(al, {f.base.base.loc, pArgs_ref});
        args_PyTuple_SetItem.push_back(al, {f.base.base.loc, ASRUtils::EXPR(ASR::make_IntegerConstant_t(al,
                                                                f.base.base.loc, i, i4_type))});
        args_PyTuple_SetItem.push_back(al, {f.base.base.loc, native_to_cpython(al, f.m_args[i], f, body)});
        std::string p = "pA" + std::to_string(i);
        s.from_str(al, p);
        ASR::asr_t *pA = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                            ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, i4_type, nullptr,
                            ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t *pA_ref = ASRUtils::EXPR(ASR::make_Var_t(al, f.base.base.loc, ASR::down_cast<ASR::symbol_t>(pA)));
        f.m_symtab->add_symbol(p, ASR::down_cast<ASR::symbol_t>(pA));
        body.push_back(al,
            ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, pA_ref,
                ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, f.base.base.loc, sym_PyTuple_SetItem, nullptr,
                            args_PyTuple_SetItem.p, args_PyTuple_SetItem.n, i4_type, nullptr, nullptr)), nullptr)));
    }

    // calling CPython Function
    ASR::symbol_t *sym_PyObject_CallObject = f.m_symtab->resolve_symbol("PyObject_CallObject");
    Vec<ASR::call_arg_t> args_PyObject_CallObject;
    args_PyObject_CallObject.reserve(al, 2);
    args_PyObject_CallObject.push_back(al, {f.base.base.loc, pFunc_ref});
    args_PyObject_CallObject.push_back(al, {f.base.base.loc, pArgs_ref});
    s.from_str(al, "pReturn");
    ASR::asr_t *pReturn = ASR::make_Variable_t(al, f.base.base.loc, f.m_symtab, s.c_str(al), nullptr, 0,
                            ASRUtils::intent_local, nullptr, nullptr, ASR::storage_typeType::Default, ptr_t, nullptr,
                            ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false);
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
        ASR::expr_t *ret_conv_result = cpython_to_native(al, pReturn_ref, ret_type, f, body);
        body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, f.base.base.loc, ret_var_ref, ret_conv_result,
                                            nullptr)));
    }

    /*
        Py_DecRef(pName);
        Py_DecRef(pArgs);
        Py_DecRef(pReturn);
    */
    ASR::symbol_t *sym_Py_DecRef = f.m_symtab->resolve_symbol("Py_DecRef");

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
    ASRUtils::get_FunctionType(f)->m_abi = ASR::abiType::Source;
    ASRUtils::get_FunctionType(f)->m_deftype = ASR::deftypeType::Implementation;
}

void pass_python_bind(Allocator &al, ASR::TranslationUnit_t &unit, const PassOptions &pass_options) {
    if (pass_options.c_skip_bindpy_pass) {
        // FIXME: C backend supports arrays, it is used in bindpy_02 to bindpy_04 tests.
        //  This pass currently does not handle any aggregate types.
        //  Once we include support for aggregate types, we should remove this check, and
        //  remove the `c_backend` variable from PassOptions.
        //  We will also need to remove the special handling of BindPython ABI in C
        //  backend as it would be handled by this ASR pass.
        return;
    }

    if (!pass_options.enable_cpython) {
        // python_bind pass is skipped if CPython is not enabled
        return;
    }

    std::vector<ASRUtils::ASRFunc> fns;
    fns.push_back({"Py_Initialize", {}, ASRUtils::VOID});
    fns.push_back({"Py_IsInitialized", {}, ASRUtils::I32});
    // fns.push_back({"PyRun_SimpleString", {STR}, ASRUtils::I32});
    fns.push_back({"Py_DecodeLocale", {ASRUtils::STR, ASRUtils::PTR}, ASRUtils::PTR});
    fns.push_back({"PySys_SetArgv", {ASRUtils::I32, ASRUtils::PTR_TO_PTR}, ASRUtils::VOID});
    fns.push_back({"Py_FinalizeEx", {}, ASRUtils::I32});
    fns.push_back({"PyUnicode_FromString", {ASRUtils::STR}, ASRUtils::PTR});
    fns.push_back({"PyUnicode_AsUTF8AndSize", {ASRUtils::PTR, ASRUtils::PTR}, ASRUtils::STR});
    fns.push_back({"PyImport_Import", {ASRUtils::PTR}, ASRUtils::PTR});
    fns.push_back({"Py_DecRef", {ASRUtils::PTR}, ASRUtils::VOID});
    fns.push_back({"Py_IncRef", {ASRUtils::PTR}, ASRUtils::VOID});
    fns.push_back({"PyObject_GetAttrString", {ASRUtils::PTR, ASRUtils::STR}, ASRUtils::PTR});
    fns.push_back({"PyTuple_New", {ASRUtils::I32}, ASRUtils::PTR});
    fns.push_back({"PyTuple_SetItem", {ASRUtils::PTR, ASRUtils::I32, ASRUtils::PTR}, ASRUtils::I32});
    fns.push_back({"PyTuple_GetItem", {ASRUtils::PTR, ASRUtils::I64}, ASRUtils::PTR});
    fns.push_back({"PyObject_CallObject", {ASRUtils::PTR, ASRUtils::PTR}, ASRUtils::PTR});
    fns.push_back({"PyLong_AsLongLong", {ASRUtils::PTR}, ASRUtils::I64});
    fns.push_back({"PyLong_AsUnsignedLongLong", {ASRUtils::PTR}, ASRUtils::U64});
    fns.push_back({"PyLong_FromLongLong", {ASRUtils::I64}, ASRUtils::PTR});
    fns.push_back({"PyLong_FromUnsignedLongLong", {ASRUtils::U64}, ASRUtils::PTR});
    fns.push_back({"PyFloat_FromDouble", {ASRUtils::F64}, ASRUtils::PTR});
    fns.push_back({"PyFloat_AsDouble", {ASRUtils::PTR}, ASRUtils::F64});
    fns.push_back({"PyBool_FromLong", {ASRUtils::I32}, ASRUtils::PTR});
    fns.push_back({"PyObject_IsTrue", {ASRUtils::PTR}, ASRUtils::I32});
    fns.push_back({"PyList_New", {ASRUtils::I64}, ASRUtils::PTR});
    fns.push_back({"PyList_Append", {ASRUtils::PTR, ASRUtils::PTR}, ASRUtils::I32});
    fns.push_back({"PyList_GetItem", {ASRUtils::PTR, ASRUtils::I64}, ASRUtils::PTR});
    fns.push_back({"PyList_Size", {ASRUtils::PTR}, ASRUtils::I64});
    fns.push_back({"PySet_New", {ASRUtils::PTR}, ASRUtils::PTR});
    fns.push_back({"PySet_Add", {ASRUtils::PTR, ASRUtils::PTR}, ASRUtils::PTR});
    fns.push_back({"PySet_Size", {ASRUtils::PTR}, ASRUtils::I64});
    fns.push_back({"PyDict_New", {}, ASRUtils::PTR});
    fns.push_back({"PyDict_SetItem", {ASRUtils::PTR, ASRUtils::PTR, ASRUtils::PTR}, ASRUtils::I32});
    fns.push_back({"PyObject_GetIter", {ASRUtils::PTR}, ASRUtils::PTR});
    fns.push_back({"PyIter_Next", {ASRUtils::PTR}, ASRUtils::PTR});

    Location *l = al.make_new<Location>();
    l->first = 0;
    l->last = 0;

    ASRUtils::declare_functions(al, fns, *l, unit.m_symtab, "Python.h");

    for (auto &item : unit.m_symtab->get_scope()) {
        if (ASR::is_a<ASR::Function_t>(*item.second)) {
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(item.second);
            if (ASRUtils::get_FunctionType(f)->m_abi == ASR::abiType::BindPython) {
                if (f->n_body == 0 && f->m_module_file) {
                    generate_body(al, *f);
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
                            generate_body(al, *f);
                        }
                    }
                }
            }
        }

    }

    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}

}  // namespace LCompilers
