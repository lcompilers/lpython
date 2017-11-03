import argparse
import os

from llvmlite import ir
from llvmlite.binding import get_default_triple
import llvmlite.binding as llvm

from .ast import ast, parse

symbol_table = {}

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

    b = bytearray((fmt + '\00').encode('ascii'))
    fmt_const = ir.Constant(ir.ArrayType(ir.IntType(8), len(b)), b)
    fmt_var = create_global_var(module, "fmt_printf", fmt_const)
    fmt_ptr = builder.bitcast(fmt_var, c_ptr)

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

class CodeGenVisitor2(ast.ASTVisitor):

    def visit_Program(self, node):
        self.module  = ir.Module()
        self.module.triple = get_default_triple()
        self.types = {
                    "integer": ir.IntType(64)
                }

        int_type = ir.IntType(64);
        fn = ir.FunctionType(int_type, [])
        self.func = ir.Function(self.module, fn, name="main")
        block = self.func.append_basic_block(name='.entry')
        self.builder = ir.IRBuilder(block)
        self.visit_sequence(node.body)
        self.builder.ret(ir.Constant(ir.IntType(64), 0))

    def visit_Declaration(self, node):
        for v in node.vars:
            sym = v.sym
            type_f = v.sym_type
            if type_f not in self.types:
                raise Exception("Type not implemented.")
            ptr = self.builder.alloca(self.types[type_f], name=sym)
            symbol_table[sym] = {"name": sym, "type": type_f, "ptr": ptr}

    def visit_Assignment(self, node):
        lhs = node.target
        if lhs not in symbol_table:
            raise Exception("Undefined variable.")
        sym = symbol_table[lhs]
        ptr = sym["ptr"]
        value = self.visit(node.value)
        if value.type != ptr.type.pointee:
            raise Exception("Type mismatch in assignment.")
        self.builder.store(value, ptr)

    def visit_Print(self, node):
        for expr in node.values:
            if isinstance(expr, ast.Str):
                continue # TODO: implement string printing
            v = self.visit(expr)
            if v.type == self.types["integer"]:
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
            lhs = ir.Constant(ir.IntType(64), 0)
            return self.builder.sub(lhs, rhs)
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
        if v not in symbol_table:
            raise Exception("Undefined variable.")
        sym = symbol_table[v]
        return self.builder.load(sym["ptr"])

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



def main():
    parser = argparse.ArgumentParser(description="Fortran compiler.")
    parser.add_argument('file', help="source file")
    parser.add_argument('--emit-llvm', action="store_true",
            help="emit LLVM IR source code, do not assemble or link")
    parser.add_argument('-S', action="store_true",
            help="emit assembly, do not assemble or link")
    parser.add_argument('-c', action="store_true",
            help="compile and assemble, do not link")
    parser.add_argument('-o', metavar="FILE",
            help="place the output into FILE")
    parser.add_argument('--ld-musl', action="store_true",
            help="invoke ld directly and link with musl")
    args = parser.parse_args()

    filename = args.file
    basename, ext = os.path.splitext(os.path.basename(filename))
    link_only = (open(filename, mode="rb").read(4)[1:] == b"ELF")
    if args.o:
        outfile = args.o
    elif args.S:
        outfile = "%s.s" % basename
    elif args.emit_llvm:
        outfile = "%s.ll" % basename
    elif args.c:
        outfile = "%s.o" % basename
    else:
        outfile = "a.out"

    if args.c:
        objfile = outfile
    elif link_only:
        objfile = filename
    else:
        objfile = "tmp_object_file.o"

    if not link_only:
        # Fortran -> LLVM IR source
        source = open(filename).read()
        ast_tree = parse(source)
        v = CodeGenVisitor2()
        v.visit(ast_tree)
        source_ll = str(v.module)
        if args.emit_llvm:
            with open(outfile, "w") as ll:
                ll.write(source_ll)
            return

        # LLVM IR source -> assembly / machine object code
        llvm.initialize()
        llvm.initialize_native_asmprinter()
        llvm.initialize_native_target()
        target = llvm.Target.from_triple(v.module.triple)
        target_machine = target.create_target_machine()
        target_machine.set_asm_verbosity(True)
        mod = llvm.parse_assembly(source_ll)
        mod.verify()
        if args.S:
            with open(outfile, "w") as o:
                o.write(target_machine.emit_assembly(mod))
            return
        with open(objfile, "wb") as o:
            o.write(target_machine.emit_object(mod))
        if args.c:
            return

    # Link object code
    if args.ld_musl:
        # Invoke `ld` directly and link with musl. This is system dependent.
        musl_dir="/usr/lib/x86_64-linux-musl"
        LDFLAGS="{0}/crt1.o {0}/libc.a".format(musl_dir)
        os.system("ld -o %s %s %s" % (outfile, objfile, LDFLAGS))
    else:
        # Invoke a C compiler to do the linking
        os.system("cc -o %s %s" % (outfile, objfile))
    if objfile == "tmp_object_file.o":
        os.system("rm %s" % objfile)
