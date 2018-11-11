from ctypes import CFUNCTYPE, c_int
from copy import deepcopy

import llvmlite.binding as llvm

from ..ast import parse, dump, SyntaxErrorException, ast
from ..semantic.analyze import SymbolTableVisitor, annotate_tree
from .gen import codegen
from ..liblfort import load_lfortran_runtime_library


class FortranEvaluator(object):

    def __init__(self):
        self.lle = LLVMEvaluator()

        self.symbol_table_visitor = SymbolTableVisitor()
        self._global_scope = self.symbol_table_visitor._global_scope

        self.anonymous_fn_counter = 0

    def parse(self, source):
        return parse(source, translation_unit=False)

    def semantic_analysis(self, ast_tree):
        self.is_expr = isinstance(ast_tree, ast.expr)
        self.is_stmt = isinstance(ast_tree, ast.stmt)
        if self.is_expr:
            # if `ast_tree` is an expression, wrap it in an anonymous function
            self.anonymous_fn_counter += 1
            self.anonymous_fn_name = "_run%d" % self.anonymous_fn_counter
            body = [ast.Assignment(ast.Name(self.anonymous_fn_name,
                lineno=1, col_offset=1),
                ast_tree, lineno=1, col_offset=1)]

            ast_tree = ast.Function(name=self.anonymous_fn_name, args=[],
                returns=None,
                decl=[], body=body, contains=[],
                lineno=1, col_offset=1)

        if self.is_stmt:
            # if `ast_tree` is a statement, wrap it in an anonymous subroutine
            self.anonymous_fn_counter += 1
            self.anonymous_fn_name = "_run%d" % self.anonymous_fn_counter
            body = [ast_tree]

            ast_tree = ast.Function(name=self.anonymous_fn_name, args=[],
                returns=None,
                decl=[], body=body, contains=[],
                lineno=1, col_offset=1)

        self.symbol_table_visitor.mark_all_external()
        self.symbol_table_visitor.visit(ast_tree)
        annotate_tree(ast_tree, self._global_scope)
        return ast_tree

    def llvm_code_generation(self, ast_tree, optimize=True):
        source_ll = str(codegen(ast_tree,
            self.symbol_table_visitor._global_scope))
        self._source_ll = source_ll
        mod = self.lle.parse(self._source_ll)
        if optimize:
            self.lle.optimize(mod)
            self._source_ll_opt = str(mod)
        else:
            self._source_ll_opt = None
        return mod

    def machine_code_generation_load_run(self, mod):
        self.lle.add_module_mod(mod)
        self._source_asm = self.lle.get_asm(mod)

        if self.is_expr:
            return self.lle.intfn(self.anonymous_fn_name)

        if self.is_stmt:
            self.lle.voidfn(self.anonymous_fn_name)
            return

    def evaluate_statement(self, ast_tree0, optimize=True):
        if ast_tree0 is None:
            return
        ast_tree = self.semantic_analysis(ast_tree0)
        mod = self.llvm_code_generation(ast_tree, optimize)
        return self.machine_code_generation_load_run(mod)

    def evaluate(self, source, optimize=True):
        ast_tree0 = self.parse(source)
        if isinstance(ast_tree0, list):
            for statement in ast_tree0:
                r = self.evaluate_statement(statement, optimize)
        else:
            r = self.evaluate_statement(ast_tree0, optimize)
        return r


class LLVMEvaluator(object):

    def __init__(self):
        llvm.initialize()
        llvm.initialize_native_asmprinter()
        llvm.initialize_native_target()

        target = llvm.Target.from_default_triple()
        self.target_machine = target.create_target_machine()
        self.target_machine.set_asm_verbosity(True)
        mod = llvm.parse_assembly("")
        mod.verify()
        self.ee = llvm.create_mcjit_compiler(mod, self.target_machine)

        pmb = llvm.create_pass_manager_builder()
        pmb.opt_level = 3
        pmb.loop_vectorize = True
        pmb.slp_vectorize = True
        self.pm = llvm.create_module_pass_manager()
        pmb.populate(self.pm)

        load_lfortran_runtime_library()

    def parse(self, source):
        mod = llvm.parse_assembly(source)
        mod.verify()
        return mod

    def optimize(self, mod):
        self.pm.run(mod)

    def add_module_mod(self, mod):
        self.ee.add_module(mod)
        self.ee.finalize_object()

    def get_asm(self, mod):
        return self.target_machine.emit_assembly(mod)

    def add_module(self, source, optimize=True):
        self.mod = self.parse(source)
        if optimize:
            self.optimize(self.mod)
            source_opt = str(self.mod)
        else:
            source_opt = None
        self.add_module_mod(self.mod)
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