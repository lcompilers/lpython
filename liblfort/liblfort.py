import argparse
import os
import sys

import llvmlite.binding as llvm

from .ast import parse
from .semantic.analyze import create_symbol_table, annotate_tree
from .codegen.gen import codegen


def main():
    parser = argparse.ArgumentParser(description="Fortran compiler.")
    parser.add_argument('file', help="source file")
    parser.add_argument('--emit-llvm', action="store_true",
            help="emit LLVM IR source code, do not assemble or link")
    parser.add_argument('-S', action="store_true",
            help="emit assembly, do not assemble or link")
    parser.add_argument('-c', action="store_true",
            help="compile and assemble, do not link")
    parser.add_argument('-o', metavar="FILE",
            help="place the output into FILE")
    parser.add_argument('--ld-musl', action="store_true",
            help="invoke ld directly and link with musl")
    args = parser.parse_args()

    filename = args.file
    basename, ext = os.path.splitext(os.path.basename(filename))
    link_only = (open(filename, mode="rb").read(4)[1:] == b"ELF")
    if args.o:
        outfile = args.o
    elif args.S:
        outfile = "%s.s" % basename
    elif args.emit_llvm:
        outfile = "%s.ll" % basename
    elif args.c:
        outfile = "%s.o" % basename
    else:
        outfile = "a.out"

    if args.c:
        objfile = outfile
    elif link_only:
        objfile = filename
    else:
        objfile = "tmp_object_file.o"

    if not link_only:
        # Fortran -> LLVM IR source
        source = open(filename).read()
        try:
            ast_tree = parse(source)
        except SyntaxErrorException as err:
            print(str(err))
            sys.exit(1)
        symbol_table = create_symbol_table(ast_tree)
        annotate_tree(ast_tree, symbol_table)
        module = codegen(ast_tree, symbol_table)
        source_ll = str(module)
        if args.emit_llvm:
            with open(outfile, "w") as ll:
                ll.write(source_ll)
            return

        # LLVM IR source -> assembly / machine object code
        llvm.initialize()
        llvm.initialize_native_asmprinter()
        llvm.initialize_native_target()
        target = llvm.Target.from_triple(module.triple)
        target_machine = target.create_target_machine()
        target_machine.set_asm_verbosity(True)
        mod = llvm.parse_assembly(source_ll)
        mod.verify()
        if args.S:
            with open(outfile, "w") as o:
                o.write(target_machine.emit_assembly(mod))
            return
        with open(objfile, "wb") as o:
            o.write(target_machine.emit_object(mod))
        if args.c:
            return

    # Link object code
    if args.ld_musl:
        # Invoke `ld` directly and link with musl. This is system dependent.
        musl_dir="/usr/lib/x86_64-linux-musl"
        LDFLAGS="{0}/crt1.o {0}/libc.a".format(musl_dir)
        os.system("ld -o %s %s %s" % (outfile, objfile, LDFLAGS))
    else:
        # Invoke a C compiler to do the linking
        os.system("cc -o %s %s" % (outfile, objfile))
    if objfile == "tmp_object_file.o":
        os.system("rm %s" % objfile)
