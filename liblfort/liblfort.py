import argparse
import os

from llvmlite import ir
from llvmlite.binding import get_default_triple
import llvmlite.binding as llvm

from .fortranParser import fortranParser
from .fortranVisitor import fortranVisitor
from .fortran_parser import get_parser

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

class CodeGenVisitor(fortranVisitor):
    def __init__(self):
        self.module  = ir.Module()
        self.module.triple = get_default_triple()

    # Visit a parse tree produced by fortranParser#program.
    def visitProgram(self, ctx:fortranParser.ProgramContext):
        int_type = ir.IntType(64);
        fn = ir.FunctionType(int_type, [])
        self.func = ir.Function(self.module, fn, name="main")
        block = self.func.append_basic_block(name='.entry')
        self.builder = ir.IRBuilder(block)
        self.visitChildren(ctx)
        self.builder.ret(ir.Constant(ir.IntType(64), 0))


    # Visit a parse tree produced by fortranParser#var_decl.
    def visitVar_decl(self, ctx:fortranParser.Var_declContext):
        for v in ctx.var_sym_decl():
            sym = v.ID().getText()
            symbol_table[sym] = {"name": sym, "type": ctx.var_type().getText()}
        return self.visitChildren(ctx)

    # Visit a parse tree produced by fortranParser#assignment_statement.
    def visitAssignment_statement(self, ctx:fortranParser.Assignment_statementContext):
        lhs = ctx.ID().getText()
        rhs = ctx.expr().getText()
        if ctx.op.text == "=>":
            raise Exception("operator => not implemented")
        assert ctx.op.text == "="
        #print("%s = %s" % (lhs, rhs))
        lhs = self.visit(ctx.ID())
        rhs = self.visit(ctx.expr())
        printf(self.module, self.builder, "num %d.\n", rhs)


    # Visit a parse tree produced by fortranParser#expr_pow.
    def visitExpr_pow(self, ctx:fortranParser.Expr_powContext):
        lhs = self.visit(ctx.expr(0))
        rhs = self.visit(ctx.expr(1))
        raise Exception("llvm doesn't have pow, not implemented yet")
        # return self.builder.pow(lhs, rhs)

    # Visit a parse tree produced by fortranParser#expr_muldiv.
    def visitExpr_muldiv(self, ctx:fortranParser.Expr_muldivContext):
        op = ctx.op.text
        lhs = self.visit(ctx.expr(0))
        rhs = self.visit(ctx.expr(1))
        if op == '*':
            return self.builder.mul(lhs, rhs)
        else:
            return self.builder.div(lhs, rhs)

    # Visit a parse tree produced by fortranParser#expr_muldiv.
    def visitExpr_addsub(self, ctx:fortranParser.Expr_muldivContext):
        op = ctx.op.text
        lhs = self.visit(ctx.expr(0))
        rhs = self.visit(ctx.expr(1))
        if op == '+':
            return self.builder.add(lhs, rhs)
        else:
            return self.builder.sub(lhs, rhs)

    # Visit a parse tree produced by fortranParser#expr_truefalse.
    def visitExpr_truefalse(self, ctx:fortranParser.Expr_truefalseContext):
        op = ctx.op.text
        a = ir.Constant(ir.IntType(64), 1)
        if op == '.true.':
            return self.builder.icmp_signed("==", a, a)
        else:
            return self.builder.icmp_signed("!=", a, a)

    # Visit a parse tree produced by fortranParser#expr_rel.
    def visitExpr_rel(self, ctx:fortranParser.Expr_relContext):
        op = ctx.op.text
        lhs = self.visit(ctx.expr(0))
        rhs = self.visit(ctx.expr(1))
        if op == "/=": op = "!="
        return self.builder.icmp_signed(op, lhs, rhs)

    # Visit a parse tree produced by fortranParser#expr_nest.
    def visitExpr_nest(self, ctx:fortranParser.Expr_nestContext):
        return self.visit(ctx.expr())

    # Visit a parse tree produced by fortranParser#number_real.
    def visitNumber_real(self, ctx:fortranParser.Number_realContext):
        num = ctx.NUMBER().getText()
        return ir.Constant(ir.IntType(64), int(num))

    # Visit a parse tree produced by fortranParser#print_statement.
    def visitPrint_statement(self, ctx:fortranParser.Print_statementContext):
        for expr in ctx.expr_list().expr():
            if isinstance(expr, fortranParser.Expr_stringContext):
                pass
            elif expr.ID():
                assert len(expr.ID()) == 1
                var = expr.ID(0).getText()
#                print("printing name=%s type=%s" % (symbol_table[var]["name"],
#                    symbol_table[var]["type"]))
            else:
                raise Exception("Can only print variables for now.")
        return self.visitChildren(ctx)

    # Visit a parse tree produced by fortranParser#stop_statement.
    def visitStop_statement(self, ctx:fortranParser.Stop_statementContext):
        if ctx.NUMBER():
            num = int(ctx.NUMBER().getText())
            exit(self.module, self.builder, num)
        else:
            raise Exception("Not implemented yet.")
        return self.visitChildren(ctx)

    # Visit a parse tree produced by fortranParser#if_single_line.
    def visitIf_single_line(self, ctx:fortranParser.If_single_lineContext):
        cond = self.visit(ctx.if_cond().expr())
        with self.builder.if_then(cond):
            self.visitChildren(ctx)

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
        parser = get_parser(source)
        tree = parser.root()
        v = CodeGenVisitor()
        v.visit(tree)
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
