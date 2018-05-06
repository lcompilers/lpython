#!/usr/bin/env python

from pygments.lexers import FortranLexer
from prompt_toolkit.history import InMemoryHistory
from prompt_toolkit.layout.lexers import PygmentsLexer
from prompt_toolkit.shortcuts import prompt, print_tokens
from prompt_toolkit.styles import style_from_dict
from prompt_toolkit.token import Token

from liblfort.ast import parse, dump, SyntaxErrorException
from liblfort.semantic.analyze import create_symbol_table, annotate_tree
from liblfort.codegen.gen import codegen

def print_bold(text):
    style = style_from_dict({
        Token.Bold: '#ansiblue bold',
    })
    tokens = [
        (Token.Bold, text),
        (Token, '\n'),
    ]
    print_tokens(tokens, style=style)

def handle_input(source):
    print_bold("Input:")
    print(source)

    print()
    try:
        ast_tree = parse(source, translation_unit=False)
    except SyntaxErrorException as err:
        print(str(err))
        return
    print_bold("AST:")
    print(dump(ast_tree))

    print()
    symbol_table = create_symbol_table(ast_tree)
    print_bold("Symbol table:")
    print(symbol_table)

    #print()
    annotate_tree(ast_tree, symbol_table)
    # TODO: implement pretty printing of the annotated tree
    #print("Annotated tree:")
    #print(ast_tree)

    print()
    module = codegen(ast_tree, symbol_table)
    source_ll = str(module)
    print_bold("LLVM IR:")
    print(source_ll)



history = InMemoryHistory()

print("Interactive Fortran.")
print("  * Use Ctrl-D to exit.")
print("  * Use Alt-Enter to submit.")
print("  * Features:")
print("    - Multi-line editing")
print("    - History")
print("    - Syntax highlighting")
print()

try:
    while True:
        text = prompt('> ', history=history, lexer=PygmentsLexer(FortranLexer),
                multiline=True)
        print()
        handle_input(text)
        print()
except EOFError:
    print("Exiting.")

