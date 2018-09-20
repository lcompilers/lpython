from ctypes import CFUNCTYPE, c_int
from copy import deepcopy

import llvmlite.binding as llvm

from ..ast import parse, dump, SyntaxErrorException, ast
from ..semantic.analyze import SymbolTableVisitor, annotate_tree
from .gen import CodeGenVisitor


class FortranEvaluator(object):

    def __init__(self):
        self.lle = LLVMEvaluator()

        self.symbol_table_visitor = SymbolTableVisitor()
        self.code_gen_visitor = CodeGenVisitor(
            self.symbol_table_visitor.symbol_table)
        self.symbol_table = self.symbol_table_visitor.symbol_table

        self.anonymous_fn_counter = 0

    def parse(self, source):
        self.ast_tree = parse(source, translation_unit=False)
        self.ast_tree0 = deepcopy(self.ast_tree)

    def semantic_analysis(self):
        self.is_expr = isinstance(self.ast_tree, ast.expr)
        self.is_stmt = isinstance(self.ast_tree, ast.stmt)
        if self.is_expr:
            # if `ast_tree` is an expression, wrap it in an anonymous function
            self.anonymous_fn_counter += 1
            self.anonymous_fn_name = "_run%d" % self.anonymous_fn_counter
            body = [ast.Assignment(ast.Name(self.anonymous_fn_name,
                lineno=1, col_offset=1),
                self.ast_tree, lineno=1, col_offset=1)]

            self.ast_tree = ast.Function(name=self.anonymous_fn_name, args=[],
                returns=None,
                decl=[], body=body, contains=[],
                lineno=1, col_offset=1)

        if self.is_stmt:
            # if `ast_tree` is a statement, wrap it in an anonymous subroutine
            self.anonymous_fn_counter += 1
            self.anonymous_fn_name = "_run%d" % self.anonymous_fn_counter
            body = [self.ast_tree]

            self.ast_tree = ast.Function(name=self.anonymous_fn_name, args=[],
                returns=None,
                decl=[], body=body, contains=[],
                lineno=1, col_offset=1)

        self.symbol_table_visitor.visit(self.ast_tree)
        annotate_tree(self.ast_tree, self.symbol_table)

    def llvm_code_generation(self):
        # TODO: keep adding to the "module", i.e., pass it as an optional
        # argument to codegen() on subsequent runs of evaluate(), also store
        # and keep appending to symbol_table.
        if not isinstance(self.ast_tree, ast.Declaration):
            self.code_gen_visitor.do_global_vars()
        self.code_gen_visitor.codegen(self.ast_tree)
        source_ll = str(self.code_gen_visitor.module)
        self._source_ll = source_ll
        # TODO:
        #
        # if not (ast_tree is expression):
        #     # The `source` is a subroutine, variable declaration, etc., just
        #     # keep appending the output and the symbol table.
        #     self._source = self._source + source_ll
        #     self._symbol_table <update using symbol_table>
        # else:
        #     # The expression was wrapped into `_run1` above, let us compile
        #     # it and run it:
        #     <everything below...>

    def machine_code_generation_load_run(self, optimize=True):
        self._source_ll_opt = self.lle.add_module(self._source_ll,
                optimize=optimize)

        if self.is_expr:
            self.code_gen_visitor = CodeGenVisitor(
                self.symbol_table_visitor.symbol_table)
            return self.lle.intfn(self.anonymous_fn_name)

        if self.is_stmt:
            self.code_gen_visitor = CodeGenVisitor(
                self.symbol_table_visitor.symbol_table)
            self.lle.voidfn(self.anonymous_fn_name)
            return

    def evaluate(self, source, optimize=True):
        self.parse(source)
        self.semantic_analysis()
        self.llvm_code_generation()
        return self.machine_code_generation_load_run(optimize)

class LLVMEvaluator(object):

    def __init__(self):
        llvm.initialize()
        llvm.initialize_native_asmprinter()
        llvm.initialize_native_target()

        self.target = llvm.Target.from_default_triple()
        target_machine = self.target.create_target_machine()
        mod = llvm.parse_assembly("")
        mod.verify()
        self.ee = llvm.create_mcjit_compiler(mod, target_machine)

    def parse(self, source):
        mod = llvm.parse_assembly(source)
        mod.verify()
        return mod

    def optimize(self, mod):
        pmb = llvm.create_pass_manager_builder()
        pmb.opt_level = 3
        pmb.loop_vectorize = True
        pmb.slp_vectorize = True
        pm = llvm.create_module_pass_manager()
        pmb.populate(pm)
        pm.run(mod)

    def add_module_mod(self, mod):
        self.ee.add_module(mod)
        self.ee.finalize_object()

    def add_module(self, source, optimize=True):
        mod = self.parse(source)
        if optimize:
            self.optimize(mod)
            source_opt = str(mod)
        else:
            source_opt = None
        self.add_module_mod(mod)
        return source_opt

    def intfn(self, name):
        """
        `name` is a string, the name of the function of no arguments that
        returns an int, the function will be called and the integer value
        returned. The `name` must be present and of the correct signature,
        otherwise the behavior is undefined.
        """
        fptr = CFUNCTYPE(c_int)(self.ee.get_function_address(name))
        return fptr()

    def voidfn(self, name):
        """
        `name` is a string, the name of the function of no arguments and no
        return value, the function will be called. The `name` must be present
        and of the correct signature, otherwise the behavior is undefined.
        """
        fptr = CFUNCTYPE(None)(self.ee.get_function_address(name))
        return fptr()