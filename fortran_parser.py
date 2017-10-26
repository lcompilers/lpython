import antlr4
from fortranParser import fortranParser
from fortranLexer import fortranLexer
from fortranListener import fortranListener

def test_fortran_parser():
    f = open("examples/m1.f90").read()
    stream = antlr4.InputStream(f)
    lex    = fortranLexer(stream)

    tokens = antlr4.CommonTokenStream(lex)
    parser = fortranParser(tokens)

    tree = parser.module()

    print(tree.toStringTree(recog=parser))

if __name__ == "__main__":
    test_fortran_parser()
