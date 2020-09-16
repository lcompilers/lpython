import unittest
import jupyter_kernel_test as jkt

class IRKernelTests(jkt.KernelTests):
    kernel_name = "fortran"

    language_name = "fortran"

    file_extension = ".f90"

    code_hello_world = 'print *, "hello, world"'
    #code_stderr = "1x"

    #complete_code_samples = ['1', 'print *, "hello, world"',
    #        "integer :: i"]
    #incomplete_code_samples = ["subroutine f("]

    #code_generate_error = "1x"

    code_execute_result = [
        {'code': "1+2+3", 'result': "6"},
        {'code': "1+2", 'result': "3"},
        {'code': "integer :: x; x = 5; x*2", 'result': "10"},
    ]

if __name__ == '__main__':
    unittest.main()
