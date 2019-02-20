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
    var.initializer = v
    return var

def create_global_const(module, base_name, v):
    """
    Create a unique global constant with the name base_name_%d.
    """
    var = create_global_var(module, base_name, v)
    var.global_constant = True
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

def transform_doloops(tree, global_scope):
    v = DoLoopTransformer()
    tree = v.visit(tree)
    # FIXME: after transforming do loops, we have to annotate types again:
    from ..semantic.analyze import annotate_tree
    annotate_tree(tree, global_scope)
    return tree

def create_global_string(module, builder, string):
    c_ptr = ir.IntType(8).as_pointer()
    b = bytearray((string + '\00').encode('ascii'))
    string_const = ir.Constant(ir.ArrayType(ir.IntType(8), len(b)), b)
    string_var = create_global_const(module, "compiler_id", string_const)
    string_ptr = builder.bitcast(string_var, c_ptr)
    return string_ptr

def is_int(node):
    if node._type == Integer():
        return True
    if isinstance(node._type, Array):
        return node._type.type_ == Integer()
    return False

_function_pointers = []
def create_callback_py(m, f, name=None):
    """
    Creates an LLVM function that calls the Python callback function `f`.

    Parameters:

    m ...... LLVM module
    f ...... Python function
    name ... the name of the LLVM function to create (if None, the name
             of the function 'f will be used)

    It uses ctypes to create a C functin pointer and embeds this pointer into
    the generated LLVM function. It returns a ctype's function type, which
    you can use to type the LLVM function pointer back into a Python function.

    The C pointer is stored in the `_function_pointers` global list to
    prevent it from being garbage collected.
    """
    from ctypes import c_int, c_void_p, CFUNCTYPE, cast
    from inspect import signature
    sig = signature(f)
    nargs = len(sig.parameters)
    if name is None:
        name = f.__name__
    ftype = CFUNCTYPE(c_int, *([c_int]*nargs))
    f2 = ftype(f)
    # Store the function pointer so that it does not get garbage collected
    _function_pointers.append(f2)
    faddr = cast(f2, c_void_p).value
    int64 = ir.IntType(64)
    ftype2 = ir.FunctionType(int64, [int64]*nargs)
    create_callback_stub(m, name, faddr, ftype2)
    return ftype

def create_callback_stub(m, name, faddr, ftype):
    """
    Creates an LLVM function that calls the callback function at memory
    address `addr`.
    """
    stub = ir.Function(m, ftype, name=name)
    builder = ir.IRBuilder(stub.append_basic_block('entry'))
    cb = builder.inttoptr(ir.Constant(ir.IntType(64), faddr),
        ftype.as_pointer())
    builder.ret(builder.call(cb, stub.args))

class CodeGenVisitor(ast.ASTVisitor):
    """
    Loop over AST and generate LLVM IR code.

    The __init__() function initializes a new LLVM module and the codegen()
    function keeps appending to it --- it can be called several times.

    Consult Fortran.asdl to see what the nodes are and their members. If a node
    is encountered that is not implemented by the visit_* method, the
    ASTVisitor base class raises an exception.
    """

    def __init__(self, global_scope):
        self._global_scope = global_scope
        self._current_scope = self._global_scope
        self.module = ir.Module()
        self.func = None
        self.builder = None
        self.types = {
                    Integer(): ir.IntType(64),
                    Real(): ir.DoubleType(),
                    Logical(): ir.IntType(1),
                }
        fn_type = ir.FunctionType(ir.DoubleType(),
                [ir.IntType(64), ir.DoubleType().as_pointer()])
        fn_sum = ir.Function(self.module, fn_type, name="_lfort_sum")
        self._global_scope.symbols["sum"]["fn"] = fn_sum
        fn_rand = ir.Function(self.module, fn_type, name="_lfort_random_number")
        self._global_scope.symbols["random_number"]["fn"] = fn_rand
        self._global_scope.symbols["abs"]["fn"] = self.module.declare_intrinsic(
                'llvm.fabs', [ir.DoubleType()])
        self._global_scope.symbols["sqrt"]["fn"] = self.module.declare_intrinsic(
                'llvm.sqrt', [ir.DoubleType()])
        self._global_scope.symbols["log"]["fn"] = self.module.declare_intrinsic(
                'llvm.log', [ir.DoubleType()])

        # plot
        fn_type = ir.FunctionType(ir.IntType(64),
                [ir.IntType(64), ir.IntType(64)])
        fn_plot = ir.Function(self.module, fn_type, name="_lfort_plot_test")
        self._global_scope.symbols["plot_test"]["fn"] = fn_plot
        fn_type = ir.FunctionType(ir.IntType(64),
                [ir.IntType(64), ir.IntType(64), ir.IntType(64)])
        fn_plot = ir.Function(self.module, fn_type, name="_lfort_plot")
        self._global_scope.symbols["plot"]["fn"] = fn_plot
        fn_type = ir.FunctionType(ir.IntType(64),
                [ir.IntType(64)])
        fn_plot = ir.Function(self.module, fn_type, name="_lfort_savefig")
        self._global_scope.symbols["savefig"]["fn"] = fn_plot
        fn_type = ir.FunctionType(ir.IntType(64), [])
        fn_plot = ir.Function(self.module, fn_type, name="_lfort_show")
        self._global_scope.symbols["show"]["fn"] = fn_plot

    def codegen(self, tree):
        """
        Generates code for `tree` and appends it into the LLVM module.
        """
        self.do_global_vars()
        if isinstance(tree, ast.Declaration):
            # `tree` is a declaration in the global scope.
            # The global variable is already defined by do_global_vars(),
            # nothing to do here.
            return
        assert isinstance(tree, (ast.Program, ast.Module, ast.Function,
            ast.Subroutine))
        tree = transform_doloops(tree, self._global_scope)
        self.visit(tree)

    def do_global_vars(self):
        for sym in self._global_scope.symbols:
            s = self._global_scope.symbols[sym]
            if s["func"]:
                if s["external"]:
                    if s["name"] in ["abs", "sqrt", "log", "sum",
                        "random_number"]:
                        # Skip these for now (they are handled in Program)
                        continue
                    if s["name"] in ["plot", "plot_test", "savefig", "show"]:
                        # Handled by plotting extension
                        continue
                    return_type = self.types[s["type"]]
                    args = []
                    for n, arg in enumerate(s["args"]):
                        _type = s["scope"].symbols[arg.arg]["type"]
                        if isinstance(_type, Array):
                            _type = _type.type_
                        args.append(self.types[_type].as_pointer())
                    fn = ir.FunctionType(return_type, args)
                    s["fn"] = ir.Function(self.module, fn, name=s["name"])
            else:
                name = s["name"] + "_0"
                assert not get_global(self.module, name)
                type_f = s["type"]
                if isinstance(type_f, Array):
                    assert len(type_f.shape) == 1
                    var_type = ir.ArrayType(self.types[type_f.type_],
                            type_f.shape[0])
                else:
                    var_type = self.types[type_f]
                var = ir.GlobalVariable(self.module, var_type, name=name)
                if not s["external"]:
                    # TODO: Undefined fails, but should work:
                    #var.initializer = ir.Undefined
                    # For now we initialize to 0:
                    if isinstance(type_f, Array):
                        var.initializer = ir.Constant(var_type,
                            [0]*type_f.shape[0])
                    else:
                        var.initializer = ir.Constant(var_type, 0)
                s["ptr"] = var

    def visit_Program(self, node):
        self._current_scope = node._scope
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

        for ssym in self._current_scope.symbols:
            assert ssym not in ["abs", "sqrt", "log", "sum", "random_number"]
            sym = self._current_scope.symbols[ssym]
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
        self._global_scope.symbols["abs"]["fn"] = self.module.declare_intrinsic(
                'llvm.fabs', [ir.DoubleType()])
        self._global_scope.symbols["sqrt"]["fn"] = self.module.declare_intrinsic(
                'llvm.sqrt', [ir.DoubleType()])
        self._global_scope.symbols["log"]["fn"] = self.module.declare_intrinsic(
                'llvm.log', [ir.DoubleType()])

        fn_type = ir.FunctionType(ir.DoubleType(),
                [ir.IntType(64), ir.DoubleType().as_pointer()])
        fn_sum = ir.Function(self.module, fn_type, name="_lfort_sum")
        self._global_scope.symbols["sum"]["fn"] = fn_sum
        fn_rand = ir.Function(self.module, fn_type, name="_lfort_random_number")
        self._global_scope.symbols["random_number"]["fn"] = fn_rand

        self.visit_sequence(node.contains)
        self.visit_sequence(node.body)
        self.builder.ret(ir.Constant(ir.IntType(64), 0))
        self._current_scope = node._scope.parent_scope

    def visit_Assignment(self, node):
        lhs = node.target
        if isinstance(lhs, ast.Name):
            sym = self._current_scope.resolve(lhs.id)

            value = self.visit(node.value)

            if "ptr" not in sym:
                # TODO: this must happen elsewhere, in XXX1.
                type_f = sym["type"]
                assert not isinstance(type_f, Array)
                if type_f not in self.types:
                    raise Exception("Type not implemented.")
            ptr = sym["ptr"]
            if value.type != ptr.type.pointee:
                raise Exception("Type mismatch in assignment.")
            self.builder.store(value, ptr)
        elif isinstance(lhs, ast.FuncCallOrArray):
            # array
            sym = self._current_scope.resolve(lhs.func)
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
            v = self.visit(expr)
            if isinstance(expr, ast.Str):
                printf(self.module, self.builder, "%s ", v)
            elif expr._type == Integer():
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
        sym = self._current_scope.resolve(v)
        return self.builder.load(sym["ptr"])

    def visit_Str(self, node):
        return create_global_string(self.module, self.builder, node.s)

    def visit_FuncCallOrArray(self, node):
        if isinstance(node._type, Array):
            # Array
            if len(node.args) != 1:
                raise NotImplementedError("Require exactly one index for now")
            idx = self.visit(node.args[0])
            # Convert 1-based indexing to 0-based
            idx = self.builder.sub(idx, ir.Constant(ir.IntType(64), 1))
            sym = self._current_scope.resolve(node.func)
            if sym["dummy"]:
                # Dummy array argument is just a pointer directly into the
                # array elements (e.g., a type i64*), so we index the pointer
                # itself (i.e., just one index in the GEP instruction):
                addr = self.builder.gep(sym["ptr"],
                        [idx])
            else:
                # Non-dummy array variable is a pointer to the array (e.g., a
                # type [4 x i64]*), so we first index the pointer as index 0,
                # and then access the array (i.e., two indinces in the GEP
                # instruction):
                addr = self.builder.gep(sym["ptr"],
                        [ir.Constant(ir.IntType(64), 0), idx])
            return self.builder.load(addr)
        else:
            # Function call
            sym = self._current_scope.resolve(node.func)
            fn = sym["fn"]
            # Temporary workarounds:
            if len(node.args) == 1 and sym["name"] in ["sum"]:
                # FIXME: for now we assume an array was passed in:
                arg = self._current_scope.resolve(node.args[0].id)
                addr = self.builder.gep(arg["ptr"],
                        [ir.Constant(ir.IntType(64), 0),
                        ir.Constant(ir.IntType(64), 0)])
                array_size = arg["ptr"].type.pointee.count
                return self.builder.call(fn,
                        [ir.Constant(ir.IntType(64), array_size), addr])
            if len(node.args) == 1 and sym["name"] in ["abs", "sqrt", "log"]:
                arg = self.visit(node.args[0])
                return self.builder.call(fn, [arg])
            if sym["name"] in ["plot", "plot_test", "savefig", "show"]:
                # FIXME: Pass by value currently:
                args = []
                for arg in node.args:
                    args.append(self.visit(arg))
                return self.builder.call(fn, args)

            args_ptr = []
            for arg in node.args:
                if isinstance(arg, ast.Name):
                    # Get a pointer to a variable
                    sym = self._current_scope.resolve(arg.id)
                    if isinstance(sym["type"], Array):
                        a_ptr = self.builder.gep(sym["ptr"],
                                [ir.Constant(ir.IntType(64), 0),
                                ir.Constant(ir.IntType(64), 0)])
                    else:
                        a_ptr = sym["ptr"]
                    args_ptr.append(a_ptr)
                else:
                    # Expression is a value, get a pointer to a copy of it
                    a_value = self.visit(arg)
                    a_ptr = self.builder.alloca(a_value.type)
                    self.builder.store(a_value, a_ptr)
                    args_ptr.append(a_ptr)
            return self.builder.call(fn, args_ptr)

    def visit_If(self, node):
        cond = self.visit(node.test)
        if not node.orelse:
            # Only the `then` branch
            with self.builder.if_then(cond):
                self.visit_sequence(node.body)
        else:
            # Both `then` and `else` branches
            with self.builder.if_else(cond) as (then, otherwise):
                with then:
                    self.visit_sequence(node.body)
                with otherwise:
                    self.visit_sequence(node.orelse)

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
        self._current_scope = node._scope
        fn = ir.FunctionType(ir.VoidType(), [ir.IntType(64).as_pointer(),
            ir.IntType(64).as_pointer()])
        func = ir.Function(self.module, fn, name=node.name)
        block = func.append_basic_block(name='.entry')
        builder = ir.IRBuilder(block)
        old = [self.func, self.builder]
        self.func, self.builder = func, builder
        for n, arg in enumerate(node.args):
            self._current_scope.resolve(arg.arg)["ptr"] = self.func.args[n]

        self.visit_sequence(node.body)
        self.builder.ret_void()

        self.func, self.builder = old
        self._current_scope = node._scope.parent_scope

    def visit_Function(self, node):
        self._current_scope = node._scope
        args = []
        for n, arg in enumerate(node.args):
            sym = self._current_scope._local_symbols[arg.arg]
            _type = sym["type"]
            if isinstance(_type, Array):
                _type = _type.type_
            args.append(self.types[_type].as_pointer())
        fn = ir.FunctionType(ir.IntType(64), args)
        func = ir.Function(self.module, fn, name=node.name)
        block = func.append_basic_block(name='.entry')
        builder = ir.IRBuilder(block)
        old = [self.func, self.builder]
        self.func, self.builder = func, builder
        for n, arg in enumerate(node.args):
            sym = self._current_scope._local_symbols[arg.arg]
            assert sym["name"] == arg.arg
            assert sym["dummy"] == True
            sym["ptr"] = self.func.args[n]
        for v in self._current_scope._local_symbols:
            sym = self._current_scope._local_symbols[v]
            if sym["dummy"]: continue
            type_f = sym["type"]
            if isinstance(type_f, Array):
                assert len(type_f.shape) == 1
                array_type = ir.ArrayType(self.types[type_f.type_],
                        type_f.shape[0])
                ptr = self.builder.alloca(array_type, name=sym["name"])
            else:
                ptr = self.builder.alloca(self.types[type_f], name=sym["name"])
            sym["ptr"] = ptr
        self._current_scope.resolve(node.name)["fn"] = func

        # Allocate the "result" variable
        retsym = self._current_scope.resolve(node.name)
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
        self._current_scope = node._scope.parent_scope

    def visit_SubroutineCall(self, node):
        if node.name in ["random_number"]:
            # FIXME: for now we assume an array was passed in:
            sym = self._current_scope.resolve(node.name)
            fn = sym["fn"]
            arg = self._current_scope.resolve(node.args[0].id)
            addr = self.builder.gep(arg["ptr"],
                    [ir.Constant(ir.IntType(64), 0),
                     ir.Constant(ir.IntType(64), 0)])
            array_size = arg["ptr"].type.pointee.count
            return self.builder.call(fn,
                    [ir.Constant(ir.IntType(64), array_size), addr])
        else:
            fn = get_global(self.module, node.name)
            assert fn is not None
            # Pass expressions by value (copying the result to a temporary variable
            # and passing a pointer to that), pass variables by reference.
            args_ptr = []
            for arg in node.args:
                if isinstance(arg, ast.Name):
                    # Get a pointer to a variable
                    sym = self._current_scope.resolve(arg.id)
                    a_ptr = sym["ptr"]
                    args_ptr.append(a_ptr)
                else:
                    # Expression is a value, get a pointer to a copy of it
                    a_value = self.visit(arg)
                    a_ptr = self.builder.alloca(a_value.type)
                    self.builder.store(a_value, a_ptr)
                    args_ptr.append(a_ptr)
            self.builder.call(fn, args_ptr)


def codegen(tree, global_scope):
    v = CodeGenVisitor(global_scope)
    v.codegen(tree)
    return v.module