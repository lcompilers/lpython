from test_import_06_mod1 import StringIO

def stringio_test(fd: StringIO, integer_asr: str):
    assert fd.a == integer_asr
