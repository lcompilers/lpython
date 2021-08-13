#!/usr/bin/env python

import shutil, os
from pathlib import Path
from tempfile import NamedTemporaryFile, TemporaryDirectory
import subprocess

import click
import tomli

from jinja2 import Environment, FileSystemLoader

@click.command()
@click.option('--compiler', default ='lfortran', prompt='Compiler',
              help='The compiler used')
@click.option('--execute/--no-execute', default=False)
@click.option('--plot/--no-plot', default=False)
@click.option('--build/--no-build', default=False)
@click.option('--genfiles', is_flag=True)
# TODO: Make groups
# TODO: Use temporary files

def mkfunc(compiler, genfiles, execute, plot, build):
    """Generate tests"""
    template_loader = FileSystemLoader('./')
    env = Environment(loader=template_loader)
    buildtemplate = env.get_template("cmake.jinja")
    tests_dict = tomli.loads(Path("gen.toml").read_bytes().decode())
    execenv = os.environ.copy()
    execenv["FC"]=compiler
    for mod, ftests in tests_dict.items():
        testtemplate = env.get_template(f"{mod}.f90.jinja")
        lfmn = f"lfortran_intrinsic_{mod}.f90"
        lfmod = Path(Path.cwd().parent / lfmn ).absolute()
        moddirname = Path(Path.cwd() / f"gentests/{mod}" ).absolute()
        Path.mkdir(moddirname, parents=True, exist_ok=True)
        for func in ftests:
            funcdirname = moddirname / func['fname']
            Path.mkdir(funcdirname, exist_ok=True)
            shutil.copy(lfmod, funcdirname)
            fn = f"{func['fname']}_test.f90"
            test_data = {
                'test_name': f"{func['fname']}_test",
                'test_files': [lfmn, fn ]
            }
            func['compiler'] = compiler
            Path.write_text(funcdirname/fn, testtemplate.render(func))
            Path.write_text(funcdirname/'CMakeLists.txt', buildtemplate.render(test_data))
            subprocess.Popen(['cmake', '.'], env=execenv, cwd=funcdirname).wait()
            if build==True:
                subprocess.Popen(['cmake', '--build', '.'], env=execenv, cwd=funcdirname).wait()
                if execute==True:
                    with open(str(funcdirname.absolute())+f"/{compiler}_{func['fname']}_output.dat", "w") as res:
                        subprocess.Popen([f"./{test_data['test_name']}"], env=execenv, cwd=funcdirname, stdout=res).wait()
                if plot==True:
                    mkplot(funcdirname.absolute()+f"/{compiler}_{func['fname']}_output.dat")

def mkplot(pathname):
    pass

if __name__ == '__main__':
    mkfunc()
