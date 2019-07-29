python grammar\asdl_py.py
python grammar\asdl_py.py grammar\ASR.asdl lfortran\asr\asr.py ..ast.utils
cd grammar
call antlr4 -Dlanguage=Python3 -no-listener -visitor fortran.g4 -o ..\lfortran\parser
cd ..
