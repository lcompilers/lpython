import antlr4
from fortranParser import fortranParser
from fortranLexer import fortranLexer
from fortranListener import fortranListener

def test_fortran_parser():
    stream = antlr4.FileStream("examples/m1.f90")
    lexer = fortranLexer(stream)
    tokens = antlr4.CommonTokenStream(lexer)
    parser = fortranParser(tokens)
    tree = parser.module()
    print(tree.toStringTree(recog=parser))

if __name__ == "__main__":
    test_fortran_parser()
