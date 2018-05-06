#!/usr/bin/env python

from pygments.lexers import FortranLexer
from prompt_toolkit.history import InMemoryHistory
from prompt_toolkit.layout.lexers import PygmentsLexer
from prompt_toolkit.shortcuts import prompt

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
        print("Input:")
        print(text)
        print()
except EOFError:
    print("Exiting.")

