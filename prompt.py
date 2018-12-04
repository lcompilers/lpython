#!/usr/bin/env python

from ctypes import CFUNCTYPE, c_double

from pygments.lexers import FortranLexer

from prompt_toolkit import PromptSession, print_formatted_text, HTML
from prompt_toolkit.lexers import PygmentsLexer
from prompt_toolkit.key_binding import KeyBindings

import llvmlite.binding as llvm

from lfortran.ast import dump, SyntaxErrorException
from lfortran.codegen.evaluator import FortranEvaluator


def print_bold(text):
    print_formatted_text(HTML('<ansiblue>%s</ansiblue>' % text))

def print_stage(text):
    print()
    print_formatted_text(HTML('<b><ansigreen>↓ Stage: %s</ansigreen></b>' \
        % text))
    print()

def handle_input(engine, evaluator, source):
    print_bold("Input:")
    print(source)


    print_stage("Parse")
    try:
        ast_tree0 = evaluator.parse(source)
    except SyntaxErrorException as e:
        print(e)
        return
    print_bold("Parse AST:")
    print(dump(ast_tree0))

    if isinstance(ast_tree0, list):
        statements = ast_tree0
    else:
        statements = [ast_tree0]

    for ast_tree0 in statements:
        print_stage("Semantic Analysis")
        ast_tree = evaluator.semantic_analysis(ast_tree0)
        print_bold("Semantic AST:")
        print(dump(ast_tree))
        print()
        print_bold("Symbol table:")
        print(list(evaluator._global_scope.symbols.keys()))

        print_stage("LLVM Code Generation")
        mod = evaluator.llvm_code_generation(ast_tree)
        print_bold("Initial LLVM IR:")
        print(evaluator._source_ll)
        print()
        print_bold("Optimized LLVM IR:")
        print(evaluator._source_ll_opt)

        print_stage("Machine Code Generation, Load and Run")
        result = evaluator.machine_code_generation_load_run(mod)
        print_bold("Machine code ASM:")
        print(evaluator._source_asm)
        print_bold("Result:")
        print(result)

kb = KeyBindings()

@kb.add('escape', 'enter')
def _(event):
    event.current_buffer.insert_text('\n')

@kb.add('enter')
def _(event):
    event.current_buffer.validate_and_handle()

def main():
    llvm.initialize()
    llvm.initialize_native_asmprinter()
    llvm.initialize_native_target()

    target = llvm.Target.from_triple(llvm.get_default_triple())
    target_machine = target.create_target_machine()
    target_machine.set_asm_verbosity(True)

    # Empty backing module
    backing_mod = llvm.parse_assembly("")
    backing_mod.verify()
    engine = llvm.create_mcjit_compiler(backing_mod, target_machine)

    print("Interactive Fortran.")
    print("  * Use Ctrl-D to exit.")
    print("  * Use Enter to submit.")
    print("  * Features:")
    print("    - Multi-line editing (use Alt-Enter)")
    print("    - History")
    print("    - Syntax highlighting")
    print()

    fortran_evaluator = FortranEvaluator()

    session = PromptSession('> ', lexer=PygmentsLexer(FortranLexer),
            multiline=True, key_bindings=kb)
    try:
        while True:
            text = session.prompt()
            print()
            handle_input(engine, fortran_evaluator, text)
            print()
    except EOFError:
        print("Exiting.")

if __name__ == "__main__":
    main()
