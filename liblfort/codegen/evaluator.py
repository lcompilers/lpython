from ctypes import CFUNCTYPE, c_int

import llvmlite.binding as llvm

from ..ast import parse, dump, SyntaxErrorException
from ..semantic.analyze import create_symbol_table, annotate_tree
from .gen import codegen


class FortranEvaluator(object):

    def __init__(self):
        llvm.initialize()
        llvm.initialize_native_asmprinter()
        llvm.initialize_native_target()

        self.target = llvm.Target.from_default_triple()

    def evaluate(self, source, optimize=True):
        ast_tree = parse(source, translation_unit=False)
        # Check if `ast_tree` is an expression and if it is, wrap it into an
        # anonymous function `_run1` here, on the AST level. Otherwise let it
        # be.
        symbol_table = create_symbol_table(ast_tree)
        annotate_tree(ast_tree, symbol_table)
        module = codegen(ast_tree, symbol_table)
        source_ll = str(module)
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
        mod = llvm.parse_assembly(source_ll)
        mod.verify()
        if optimize:
            pmb = llvm.create_pass_manager_builder()
            pmb.opt_level = 3
            pmb.loop_vectorize = True
            pmb.slp_vectorize = True
            pm = llvm.create_module_pass_manager()
            pmb.populate(pm)
            pm.run(mod)
        target_machine = self.target.create_target_machine()
        with llvm.create_mcjit_compiler(llvmmod, target_machine) as ee:
            ee.finalize_object()
            fptr = CFUNCTYPE(c_int)(ee.get_function_address("_run1"))
            result = fptr()
            return result
