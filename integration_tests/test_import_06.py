from test_import_06_mod1 import StringIO
from test_import_06_mod2 import stringio_test

if __name__ == '__main__':
    integer_asr : str = '(Integer 4 [])'
    fd   : StringIO = StringIO(integer_asr)
    stringio_test(fd, integer_asr)
    print("Ok")
