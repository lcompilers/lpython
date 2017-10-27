import antlr4
from antlr4.error.ErrorListener import ErrorListener
from fortranParser import fortranParser
from fortranLexer import fortranLexer
from fortranListener import fortranListener

class SyntaxErrorException(Exception):
    pass

class VerboseListener(ErrorListener) :
    def syntaxError(self, recognizer, offendingSymbol, line, column, msg, e):
        stack = recognizer.getRuleInvocationStack()
        stack.reverse()
        print("rule stack: ", str(stack))
        print("line", line, ":", column, "at", offendingSymbol, ":", msg)
        raise SyntaxErrorException("Syntax error.")

def parse(source, rule="root"):
    stream = antlr4.InputStream(source)
    lexer = fortranLexer(stream)
    tokens = antlr4.CommonTokenStream(lexer)
    parser = fortranParser(tokens)
    parser.removeErrorListeners()
    parser.addErrorListener(VerboseListener())
    try:
        tree = getattr(parser, rule)()
    except SyntaxErrorException:
        return None
    return tree.toStringTree(recog=parser)

def parse_file(filename):
    source = open(filename).read()
    return parse(source)

if __name__ == "__main__":
    print(parse_file("examples/m1.f90"))
