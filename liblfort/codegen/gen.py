from llvmlite import ir
from llvmlite.binding import get_default_triple

from ..ast import ast
from ..ast.utils import NodeTransformer
from ..semantic.analyze import Integer, Real, Logical, Array

def get_global(module, name):
    try:
        return module.get_global(name)
    except KeyError:
        return None

def create_global_var(module, base_name, v):
    """
    Create a unique global variable with the name base_name_%d.
    """
    count = 0
    while get_global(module, "%s_%d" % (base_name, count)):
        count += 1
    var = ir.GlobalVariable(module, v.type, name="%s_%d" % (base_name, count))
    var.global_constant = True
    var.initializer = v
    return var

def printf(module, builder, fmt, *args):
    """
    Call printf(fmt, *args).
    """
    c_ptr = ir.IntType(8).as_pointer()

    fmt_ptr = create_global_string(module, builder, fmt)
    fn_printf = get_global(module, "printf")
    if not fn_printf:
        fn_type = ir.FunctionType(ir.IntType(32), [c_ptr], var_arg=True)
        fn_printf = ir.Function(module, fn_type, name="printf")

    builder.call(fn_printf, [fmt_ptr] + list(args))

def exit(module, builder, n=0):
    """
    Call exit(n).
    """
    c_ptr = ir.IntType(8).as_pointer()

    n_ = ir.Constant(ir.IntType(64), n)
    fn_exit = get_global(module, "exit")
    if not fn_exit:
        fn_type = ir.FunctionType(ir.VoidType(), [ir.IntType(64)])
        fn_exit = ir.Function(module, fn_type, name="exit")

    builder.call(fn_exit, [n_])

def is_positive(expr):
    if isinstance(expr, ast.Num):
        return float(expr.n) > 0
    if isinstance(expr, ast.UnaryOp):
        if isinstance(expr.op, ast.USub):
            return not is_positive(expr.operand)
    raise Exception("Not implemented.")

class DoLoopTransformer(NodeTransformer):

    def visit_DoLoop(self, node):
        if node.head:
            if node.head.increment:
                step = node.head.increment
                if is_positive(step):
                    op = ast.LtE()
                else:
                    op = ast.GtE()
            else:
                step = ast.Num(n="1", lineno=1, col_offset=1)
                op = ast.LtE()
            cond = ast.Compare(
                ast.BinOp(ast.Name(node.head.var, lineno=1, col_offset=1),
                    ast.Add(), step, lineno=1, col_offset=1),
                op, node.head.end, lineno=1, col_offset=1)
        else:
            cond = ast.Constant(True, lineno=1, col_offset=1)
        body = self.visit_sequence(node.body)
        var_name = ast.Name(node.head.var, lineno=1, col_offset=1)
        body = [ast.Assignment(var_name,
            ast.BinOp(ast.Name(node.head.var, lineno=1, col_offset=1),
                ast.Add(), step, lineno=1, col_offset=1), lineno=1,
            col_offset=1)] + body
        return [ast.Assignment(var_name,
            ast.BinOp(node.head.start,
                ast.Sub(), step, lineno=1, col_offset=1),
            lineno=1, col_offset=1),
                ast.WhileLoop(cond, body, lineno=1, col_offset=1)]

def transform_doloops(tree, symbol_table):
    v = DoLoopTransformer()
    tree = v.visit(tree)
    # FIXME: after transforming do loops, we have to annotate types again:
    from ..semantic.analyze import annotate_tree
    annotate_tree(tree, symbol_table)
    return tree

def create_global_string(module, builder, string):
    c_ptr = ir.IntType(8).as_pointer()
    b = bytearray((string + '\00').encode('ascii'))
    string_const = ir.Constant(ir.ArrayType(ir.IntType(8), len(b)), b)
    string_var = create_global_var(module, "compiler_id", string_const)
    string_ptr = builder.bitcast(string_var, c_ptr)
    return string_ptr

def is_int(node):
    if node._type == Integer():
        return True
    if isinstance(node._type, Array):
        return node._type.type_ == Integer()
    return False

class CodeGenVisitor(ast.ASTVisitor):
    """
    Loop over AST and generate LLVM IR code.

    The __init__() function initializes a new LLVM module and the codegen()
    function keeps appending to it --- it can be called several times.

    Consult Fortran.asdl to see what the nodes are and their members. If a node
    is encountered that is not implemented by the visit_* method, the
    ASTVisitor base class raises an exception.
    """

    def __init__(self, symbol_table):
        self.symbol_table = symbol_table
        self.module = ir.Module()
        self.func = None
        self.builder = None
        self.types = {
                    Integer(): ir.IntType(64),
                    Real(): ir.DoubleType(),
                    Logical(): ir.IntType(1),
                }

    def codegen(self, tree):
        """
        Generates code for `tree` and appends it into the LLVM module.
        """
        assert isinstance(tree, (ast.Program, ast.Module, ast.Function,
            ast.Subroutine))
        tree = transform_doloops(tree, self.symbol_table)
        self.visit(tree)

    def visit_Program(self, node):
        self.module  = ir.Module()
        self.module.triple = get_default_triple()
        self.types = {
                    Integer(): ir.IntType(64),
                    Real(): ir.DoubleType(),
                    Logical(): ir.IntType(1),
                }

        int_type = ir.IntType(64)
        fn = ir.FunctionType(int_type, [])
        self.func = ir.Function(self.module, fn, name="main")
        block = self.func.append_basic_block(name='.entry')
        self.builder = ir.IRBuilder(block)

        for ssym in self.symbol_table:
            if ssym in ["abs", "sqrt", "log", "sum", "random_number"]: continue
            sym = self.symbol_table[ssym]
            type_f = sym["type"]
            if isinstance(type_f, Array):
                assert len(type_f.shape) == 1
                array_type = ir.ArrayType(self.types[type_f.type_],
                        type_f.shape[0])
                ptr = self.builder.alloca(array_type, name=sym["name"])
            else:
                if type_f not in self.types:
                    raise Exception("Type not implemented.")
                ptr = self.builder.alloca(self.types[type_f], name=sym["name"])
            sym["ptr"] = ptr
        self.symbol_table["abs"]["fn"] = self.module.declare_intrinsic(
                'llvm.fabs', [ir.DoubleType()])
        self.symbol_table["sqrt"]["fn"] = self.module.declare_intrinsic(
                'llvm.sqrt', [ir.DoubleType()])
        self.symbol_table["log"]["fn"] = self.module.declare_intrinsic(
                'llvm.log', [ir.DoubleType()])

        fn_type = ir.FunctionType(ir.DoubleType(),
                [ir.IntType(64), ir.DoubleType().as_pointer()])
        fn_sum = ir.Function(self.module, fn_type, name="_lfort_sum")
        self.symbol_table["sum"]["fn"] = fn_sum
        fn_rand = ir.Function(self.module, fn_type, name="_lfort_random_number")
        self.symbol_table["random_number"]["fn"] = fn_rand

        self.visit_sequence(node.contains)
        self.visit_sequence(node.body)
        self.builder.ret(ir.Constant(ir.IntType(64), 0))

    def visit_Assignment(self, node):
        lhs = node.target
        if isinstance(lhs, ast.Name):
            if lhs.id not in self.symbol_table:
                raise Exception("Undefined variable.")
            sym = self.symbol_table[lhs.id]
            ptr = sym["ptr"]
            value = self.visit(node.value)
            if value.type != ptr.type.pointee:
                raise Exception("Type mismatch in assignment.")
            self.builder.store(value, ptr)
        elif isinstance(lhs, ast.FuncCallOrArray):
            # array
            sym = self.symbol_table[lhs.func]
            ptr = sym["ptr"]
            assert len(sym["type"].shape) == 1
            idx = self.visit(lhs.args[0])
            value = self.visit(node.value)
            # Convert 1-based indexing to 0-based
            idx = self.builder.sub(idx, ir.Constant(ir.IntType(64), 1))
            addr = self.builder.gep(ptr, [ir.Constant(ir.IntType(64), 0),
                    idx])
            self.builder.store(value, addr)
        else:
            # should not happen
            raise Exception("`node` must be either a variable or an array")

    def visit_Print(self, node):
        for expr in node.values:
            if isinstance(expr, ast.Str):
                continue # TODO: implement string printing
            v = self.visit(expr)
            if expr._type == Integer():
                printf(self.module, self.builder, "%d ", v)
            elif expr._type == Real():
                printf(self.module, self.builder, "%.17e ", v)
            else:
                raise Exception("Type not implemented in print.")
        printf(self.module, self.builder, "\n")

    def visit_BinOp(self, node):
        op = node.op
        lhs = self.visit(node.left)
        rhs = self.visit(node.right)
        if is_int(node.left) and is_int(node.right):
            if isinstance(op, ast.Mul):
                return self.builder.mul(lhs, rhs)
            elif isinstance(op, ast.Div):
                return self.builder.udiv(lhs, rhs)
            elif isinstance(op, ast.Add):
                return self.builder.add(lhs, rhs)
            elif isinstance(op, ast.Sub):
                return self.builder.sub(lhs, rhs)
            else:
                raise Exception("Not implemented")
        else:
            if isinstance(op, ast.Mul):
                return self.builder.fmul(lhs, rhs)
            elif isinstance(op, ast.Div):
                return self.builder.fdiv(lhs, rhs)
            elif isinstance(op, ast.Add):
                return self.builder.fadd(lhs, rhs)
            elif isinstance(op, ast.Sub):
                return self.builder.fsub(lhs, rhs)
            else:
                raise Exception("Not implemented")

    def visit_UnaryOp(self, node):
        op = node.op
        rhs = self.visit(node.operand)
        if is_int(node.operand) or node.operand._type == Logical():
            if isinstance(op, ast.UAdd):
                return rhs
            elif isinstance(op, ast.USub):
                return self.builder.neg(rhs)
            elif isinstance(op, ast.Not):
                return self.builder.not_(rhs)
            else:
                raise Exception("Not implemented")
        else:
            if isinstance(op, ast.UAdd):
                return rhs
            elif isinstance(op, ast.USub):
                return self.builder.fsub(ir.Constant(ir.DoubleType(), 0), rhs)
            else:
                raise Exception("Not implemented")

    def visit_Num(self, node):
        return ir.Constant(self.types[node._type], node.o)

    def visit_Constant(self, node):
        if node.value == True:
            return ir.Constant(ir.IntType(1), "true")
        elif node.value == False:
            return ir.Constant(ir.IntType(1), "false")
        else:
            raise Exception("Not implemented")

    def visit_Name(self, node):
        v = node.id
        if v not in self.symbol_table:
            raise Exception("Undefined variable.")
        sym = self.symbol_table[v]
        return self.builder.load(sym["ptr"])

    def visit_FuncCallOrArray(self, node):
        if isinstance(node._type, Array):
            # Array
            if len(node.args) != 1:
                raise NotImplementedError("Require exactly one index for now")
            idx = self.visit(node.args[0])
            # Convert 1-based indexing to 0-based
            idx = self.builder.sub(idx, ir.Constant(ir.IntType(64), 1))
            sym = self.symbol_table[node.func]
            addr = self.builder.gep(sym["ptr"],
                    [ir.Constant(ir.IntType(64), 0), idx])
            return self.builder.load(addr)
        else:
            # Function call
            if len(node.args) != 1:
                raise NotImplementedError("Require exactly one fn arg for now")
            sym = self.symbol_table[node.func]
            fn = sym["fn"]
            arg = self.visit(node.args[0])
            if sym["name"] in ["sum"]:
                # FIXME: for now we assume an array was passed in:
                arg = self.symbol_table[node.args[0].id]
                addr = self.builder.gep(arg["ptr"],
                        [ir.Constant(ir.IntType(64), 0),
                         ir.Constant(ir.IntType(64), 0)])
                array_size = arg["ptr"].type.pointee.count
                return self.builder.call(fn,
                        [ir.Constant(ir.IntType(64), array_size), addr])
            else:
                return self.builder.call(fn, [arg])

    def visit_If(self, node):
        cond = self.visit(node.test)
        with self.builder.if_then(cond):
            self.visit_sequence(node.body)

    def visit_Compare(self, node):
        op = node.op
        lhs = self.visit(node.left)
        rhs = self.visit(node.right)
        ops = {
            ast.Eq: "==",
            ast.NotEq: "!=",
            ast.Lt: "<",
            ast.LtE: "<=",
            ast.Gt: ">",
            ast.GtE: ">=",
        }
        if is_int(node.left) and is_int(node.right):
            return self.builder.icmp_signed(ops[type(op)], lhs, rhs)
        else:
            return self.builder.fcmp_ordered(ops[type(op)], lhs, rhs)

    def visit_Stop(self, node):
        num = int(node.code)
        exit(self.module, self.builder, num)

    def visit_ErrorStop(self, node):
        exit(self.module, self.builder, 1)

    def visit_Exit(self, node):
        self.builder.branch(self.loopend)

    def visit_Cycle(self, node):
        self.builder.branch(self.loophead)

    def visit_WhileLoop(self, node):
        loophead = self.func.append_basic_block('loop.header')
        loopbody = self.func.append_basic_block('loop.body')
        loopend = self.func.append_basic_block('loop.end')
        self.loophead = loophead
        self.loopend = loopend

        # header
        self.builder.branch(loophead)
        self.builder.position_at_end(loophead)
        cond = self.visit(node.test)
        self.builder.cbranch(cond, loopbody, loopend)

        # body
        self.builder.position_at_end(loopbody)
        self.visit_sequence(node.body)
        self.builder.branch(loophead)

        # end
        self.builder.position_at_end(loopend)

    def visit_Subroutine(self, node):
        fn = ir.FunctionType(ir.VoidType(), [ir.IntType(64).as_pointer(),
            ir.IntType(64).as_pointer()])
        func = ir.Function(self.module, fn, name=node.name)
        block = func.append_basic_block(name='.entry')
        builder = ir.IRBuilder(block)
        old = [self.func, self.builder]
        self.func, self.builder = func, builder
        for n, arg in enumerate(node.args):
            self.symbol_table[arg.arg]["ptr"] = self.func.args[n]

        self.visit_sequence(node.body)
        self.builder.ret_void()

        self.func, self.builder = old

    def visit_Function(self, node):
        fn = ir.FunctionType(ir.IntType(64), [ir.IntType(64).as_pointer(),
            ir.IntType(64).as_pointer()])
        func = ir.Function(self.module, fn, name=node.name)
        block = func.append_basic_block(name='.entry')
        builder = ir.IRBuilder(block)
        old = [self.func, self.builder]
        self.func, self.builder = func, builder
        for n, arg in enumerate(node.args):
            self.symbol_table[arg.arg]["ptr"] = self.func.args[n]

        # Allocate the "result" variable
        retsym = self.symbol_table[node.name]
        type_f = retsym["type"]
        if isinstance(type_f, Array):
            raise NotImplementedError("Return arrays are not implemented yet.")
        else:
            if type_f not in self.types:
                raise Exception("Type not implemented.")
            ptr = self.builder.alloca(self.types[type_f], name=retsym["name"])
        retsym["ptr"] = ptr

        self.visit_sequence(node.body)
        retval = self.builder.load(retsym["ptr"])
        self.builder.ret(retval)

        self.func, self.builder = old

    def visit_SubroutineCall(self, node):
        if node.name in ["random_number"]:
            # FIXME: for now we assume an array was passed in:
            sym = self.symbol_table[node.name]
            fn = sym["fn"]
            arg = self.symbol_table[node.args[0].id]
            addr = self.builder.gep(arg["ptr"],
                    [ir.Constant(ir.IntType(64), 0),
                     ir.Constant(ir.IntType(64), 0)])
            array_size = arg["ptr"].type.pointee.count
            return self.builder.call(fn,
                    [ir.Constant(ir.IntType(64), array_size), addr])
        else:
            fn = get_global(self.module, node.name)
            # Pass expressions by value (copying the result to a temporary variable
            # and passing a pointer to that), pass variables by reference.
            args_ptr = []
            for arg in node.args:
                if isinstance(arg, ast.Name):
                    # Get a pointer to a variable
                    v = arg.id
                    assert v in self.symbol_table
                    sym = self.symbol_table[v]
                    a_ptr = sym["ptr"]
                    args_ptr.append(a_ptr)
                else:
                    # Expression is a value, get a pointer to a copy of it
                    a_value = self.visit(arg)
                    a_ptr = self.builder.alloca(a_value.type)
                    self.builder.store(a_value, a_ptr)
                    args_ptr.append(a_ptr)
            self.builder.call(fn, args_ptr)


def codegen(tree, symbol_table):
    v = CodeGenVisitor(symbol_table)
    v.codegen(tree)
    return v.module
