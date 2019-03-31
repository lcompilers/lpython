from lfortran import src_to_ast, ast_to_asr
from lfortran.asr.pprint import pprint_asr_str

def test_pprint1():
    src = """\
integer function f(x) result(r)
integer, intent(in) :: x
r = x+x
end function
"""
    asr = ast_to_asr(src_to_ast(src, translation_unit=False))
    s = pprint_asr_str(asr)
