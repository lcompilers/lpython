#!/usr/bin/env python

from ctypes import CFUNCTYPE, c_double

from pygments.lexers import FortranLexer

from prompt_toolkit.history import InMemoryHistory
from prompt_toolkit.layout.lexers import PygmentsLexer
from prompt_toolkit.shortcuts import prompt, print_tokens
from prompt_toolkit.styles import style_from_dict
from prompt_toolkit.token import Token

import llvmlite.binding as llvm

from liblfort.ast import parse, dump, SyntaxErrorException
from liblfort.codegen.evaluator import FortranEvaluator
from liblfort.codegen.gen import codegen
from liblfort.semantic.analyze import create_symbol_table, annotate_tree


def print_bold(text):
    style = style_from_dict({
        Token.Bold: '#ansiblue bold',
    })
    tokens = [
        (Token.Bold, text),
        (Token, '\n'),
    ]
    print_tokens(tokens, style=style)

def handle_input(engine, evaluator, source):
    print_bold("Input:")
    print(source)

    result = evaluator.evaluate(source)

    print()
    print_bold("AST:")
    print(dump(evaluator.ast_tree))

    print()
    print_bold("Symbol table:")
    print(evaluator.symbol_table)

    print()
    print_bold("LLVM IR:")
    print(evaluator._source_ll)

    print_bold("Result:")
    print(result)

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

    history = InMemoryHistory()

    print("Interactive Fortran.")
    print("  * Use Ctrl-D to exit.")
    print("  * Use Alt-Enter to submit.")
    print("  * Features:")
    print("    - Multi-line editing")
    print("    - History")
    print("    - Syntax highlighting")
    print()

    fortran_evaluator = FortranEvaluator()

    try:
        while True:
            text = prompt('> ', history=history,
                    lexer=PygmentsLexer(FortranLexer), multiline=True)
            print()
            handle_input(engine, fortran_evaluator, text)
            print()
    except EOFError:
        print("Exiting.")

if __name__ == "__main__":
    main()
