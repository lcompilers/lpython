import antlr4
from antlr4.error.ErrorListener import ErrorListener
from .parser.fortranParser import fortranParser
from .parser.fortranLexer import fortranLexer

class SyntaxErrorException(Exception):
    pass

def get_line(s, l):
    return s.split("\n")[l-1]

class VerboseErrorListener(ErrorListener):
    def syntaxError(self, recognizer, offendingSymbol, line, column, msg, e):
        stack = recognizer.getRuleInvocationStack()
        stack.reverse()
        print("rule stack: ", str(stack))
        print("%d:%d: %s" % (line, column, msg))
        print(get_line(self.source, line))
        print(" "*(column-1) + "^")
        raise SyntaxErrorException("Syntax error.")

def get_parser(source):
    stream = antlr4.InputStream(source)
    lexer = fortranLexer(stream)
    tokens = antlr4.CommonTokenStream(lexer)
    parser = fortranParser(tokens)
    parser.removeErrorListeners()
    err = VerboseErrorListener()
    err.source = source
    parser.addErrorListener(err)
    return parser

def parse(source, rule="root"):
    parser = get_parser(source)
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
