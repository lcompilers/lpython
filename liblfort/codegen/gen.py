from llvmlite import ir
from llvmlite.binding import get_default_triple

from ..ast import ast
from ..ast.utils import NodeTransformer
from ..semantic.analyze import Integer, Logical, Array

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
            cond = ast.Compare(ast.Name(node.head.var,
                lineno=1, col_offset=1),
                    op, node.head.end, lineno=1, col_offset=1)
        else:
            cond = ast.Constant(True, lineno=1, col_offset=1)
        body = self.visit_sequence(node.body)
        var_name = ast.Name(node.head.var, lineno=1, col_offset=1)
        body = body + [ast.Assignment(var_name,
            ast.BinOp(ast.Name(node.head.var, lineno=1, col_offset=1),
                ast.Add(), step, lineno=1, col_offset=1), lineno=1,
            col_offset=1)]
        return [ast.Assignment(var_name, node.head.start,
            lineno=1, col_offset=1),
                ast.WhileLoop(cond, body, lineno=1, col_offset=1)]

def transform_doloops(tree):
    v = DoLoopTransformer()
    return v.visit(tree)

def create_global_string(module, builder, string):
    c_ptr = ir.IntType(8).as_pointer()
    b = bytearray((string + '\00').encode('ascii'))
    string_const = ir.Constant(ir.ArrayType(ir.IntType(8), len(b)), b)
    string_var = create_global_var(module, "compiler_id", string_const)
    string_ptr = builder.bitcast(string_var, c_ptr)
    return string_ptr


class CodeGenVisitor(ast.ASTVisitor):
    """
    Loop over AST.

    Consult Fortran.asdl to see what the nodes are and their members. If a node
    is encountered that is not implemented by the visit_* method, the
    ASTVisitor base class raises an exception.
    """

    def __init__(self, symbol_table):
        self.symbol_table = symbol_table

    def visit_Program(self, node):
        self.module  = ir.Module()
        self.module.triple = get_default_triple()
        self.types = {
                    Integer(): ir.IntType(64),
                    Logical(): ir.IntType(1),
                }

        int_type = ir.IntType(64);
        fn = ir.FunctionType(int_type, [])
        self.func = ir.Function(self.module, fn, name="main")
        block = self.func.append_basic_block(name='.entry')
        self.builder = ir.IRBuilder(block)

        for ssym in self.symbol_table:
            sym = self.symbol_table[ssym]
            type_f = sym["type"]
            if isinstance(type_f, Array):
                assert type_f.type_ == Integer()
                assert len(type_f.shape) == 1
                array_type = ir.ArrayType(ir.IntType(64), type_f.shape[0])
                ptr = self.builder.alloca(array_type, name=sym["name"])
            else:
                if type_f not in self.types:
                    raise Exception("Type not implemented.")
                ptr = self.builder.alloca(self.types[type_f], name=sym["name"])
            sym["ptr"] = ptr

        self.visit_sequence(node.body)
        self.visit_sequence(node.contains)
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
            addr = self.builder.gep(ptr, [ir.Constant(ir.IntType(64), 0),
                    idx])
            self.builder.store(value, addr)

            printf(self.module, self.builder,
                    "array assignment: %s(%s) = %s RHS=%%d\n" % (lhs.func,
                        lhs.args[0].id,
                        node.value.id), value)
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
            else:
                raise Exception("Type not implemented in print.")
        printf(self.module, self.builder, "\n")

    def visit_BinOp(self, node):
        op = node.op
        lhs = self.visit(node.left)
        rhs = self.visit(node.right)
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

    def visit_UnaryOp(self, node):
        op = node.op
        rhs = self.visit(node.operand)
        if isinstance(op, ast.UAdd):
            return rhs
        elif isinstance(op, ast.USub):
            return self.builder.neg(rhs)
        elif isinstance(op, ast.Not):
            return self.builder.not_(rhs)
        else:
            raise Exception("Not implemented")

    def visit_Num(self, node):
        num = node.n
        return ir.Constant(ir.IntType(64), int(num))

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
        if len(node.args) != 1:
            raise NotImplementedError("Require exactly one index for now")
        idx = self.visit(node.args[0])
        sym = self.symbol_table[node.func]
        addr = self.builder.gep(sym["ptr"],
                [ir.Constant(ir.IntType(64), 0), idx])
        return self.builder.load(addr)

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
        return self.builder.icmp_signed(ops[type(op)], lhs, rhs)

    def visit_Stop(self, node):
        num = int(node.code)
        exit(self.module, self.builder, num)

    def visit_ErrorStop(self, node):
        exit(self.module, self.builder, 1)

    def visit_WhileLoop(self, node):
        loophead = self.func.append_basic_block('loop.header')
        loopbody = self.func.append_basic_block('loop.body')
        loopend = self.func.append_basic_block('loop.end')

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

def codegen(tree, symbol_table):
    tree = transform_doloops(tree)
    v = CodeGenVisitor(symbol_table)
    v.visit(tree)
    return v.module
