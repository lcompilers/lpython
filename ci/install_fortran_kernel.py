import json
import os
import sys

from jupyter_client.kernelspec import KernelSpecManager
from IPython.utils.tempdir import TemporaryDirectory

lfortran_bin = os.path.join(sys.prefix, "bin", "lfortran")

kernel_json = {
    "argv": [lfortran_bin, "--kernel", "{connection_file}"],
    "display_name": "Fortran (LFortran 0.1)",
    "language": "fortran",
}

def install_my_kernel_spec(prefix):
    print("Prefix:", prefix)
    with TemporaryDirectory() as td:
        os.chmod(td, 0o755) # Starts off as 700, not user readable
        with open(os.path.join(td, 'kernel.json'), 'w') as f:
            json.dump(kernel_json, f, sort_keys=True)

        print('Installing Jupyter kernel spec')
        KernelSpecManager().install_kernel_spec(td, 'fortran', user=False,
                prefix=prefix)

def main():
    install_my_kernel_spec(sys.prefix)

if __name__ == '__main__':
    main()
