from llvmlite import ir

from ..asr import asr
from ..semantic import kinds

def asr_type_to_llvm(type):
    """
    Converts an ASR type to an LLVM type.
    """
    if isinstance(type, asr.Integer):
        if type.kind == kinds.int64:
            return ir.IntType(64)
        elif type.kind == kinds.int32:
            return ir.IntType(32)
        else:
            raise NotImplementedError("Integer kind not implemented")
    elif isinstance(type, asr.Real):
        if type.kind == kinds.real64:
            return ir.DoubleType(64)
        elif type.kind == kinds.real32:
            return ir.DoubleType(32)
        else:
            raise NotImplementedError("Real kind not implemented")
    elif isinstance(type, asr.Logical):
        return ir.IntType(1)
    raise NotImplementedError("Type not implemented")

class ASR2LLVMVisitor(asr.ASTVisitor):

    def visit_TranslationUnit(self, node):
        self._sym2ptr = {}
        self._sym2argn = {}
        self._module = ir.Module()
        self._func = None
        self._builder = None

        for s in node.global_scope.symbols:
            sym = node.global_scope.symbols[s]
            self.visit(sym)
        for item in node.items:
            self.visit(item)

    def visit_Assignment(self, node):
        if isinstance(node.target, asr.Variable):
            target = self.visit(node.target)
            value = self.visit(node.value)
            ptr = self._sym2ptr[node.target]
            assert value.type == ptr.type.pointee
            self._builder.store(value, ptr)
        else:
            raise NotImplementedError("Array")

    def visit_BinOp(self, node):
        op = node.op
        lhs = self.visit(node.left)
        rhs = self.visit(node.right)
        if isinstance(node.type, asr.Integer):
            if isinstance(op, asr.Mul):
                return self._builder.mul(lhs, rhs)
            elif isinstance(op, asr.Div):
                return self._builder.udiv(lhs, rhs)
            elif isinstance(op, asr.Add):
                return self._builder.add(lhs, rhs)
            elif isinstance(op, asr.Sub):
                return self._builder.sub(lhs, rhs)
            else:
                raise NotImplementedError("Pow")
        else:
            if isinstance(op, asr.Mul):
                return self._builder.fmul(lhs, rhs)
            elif isinstance(op, asr.Div):
                return self._builder.fdiv(lhs, rhs)
            elif isinstance(op, asr.Add):
                return self._builder.fadd(lhs, rhs)
            elif isinstance(op, asr.Sub):
                return self._builder.fsub(lhs, rhs)
            else:
                raise NotImplementedError("Pow")

    def visit_Variable(self, node):
        return self._builder.load(self._sym2ptr[node])

    def visit_Function(self, node):
        args = []
        for n, arg in enumerate(node.args):
            llvm_type = asr_type_to_llvm(arg.type)
            args.append(llvm_type.as_pointer())
            self._sym2argn[arg] = n
        return_type = asr_type_to_llvm(node.return_var.type)
        fn = ir.FunctionType(return_type, args)
        func = ir.Function(self._module, fn, name=node.name)
        block = func.append_basic_block(name='.entry')
        builder = ir.IRBuilder(block)
        for s in node.symtab.symbols:
            sym = node.symtab.symbols[s]
            if sym.dummy and sym != node.return_var:
                ptr = func.args[self._sym2argn[sym]]
            else:
                llvm_type = asr_type_to_llvm(sym.type)
                ptr = builder.alloca(llvm_type, name=sym.name)
            self._sym2ptr[sym] = ptr

        old = [self._func, self._builder]
        self._func, self._builder = func, builder

        self.visit_sequence(node.body)
        retval = self._builder.load(self._sym2ptr[node.return_var])
        self._builder.ret(retval)

        self._func, self._builder = old





def asr_to_llvm(a):
    assert isinstance(a, asr.TranslationUnit)
    v = ASR2LLVMVisitor()
    v.visit(a)
    return v._module
