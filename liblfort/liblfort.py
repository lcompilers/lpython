import argparse
import os
import sys

import llvmlite.binding as llvm

def get_lib_path():
    import pkg_resources
    try:
        # Try the installed version
        liblfort_so = pkg_resources.resource_filename("lfortran",
            "share/lfortran/lib/liblfort.so")
    except ModuleNotFoundError:
        # Fallback to the version in git/tarball that is not installed
        here = os.path.dirname(__file__)
        base_dir = os.path.abspath(os.path.join(here, "..", "share",
            "lfortran", "lib"))
        liblfort_so = os.path.join(base_dir, "liblfort.so")
    return os.path.dirname(liblfort_so)

_lfortran_runtime_library_loaded = False

def load_lfortran_runtime_library():
    """
    Locates and loads the LFortran runtime library.

    If the library is already loaded, it will not load it again.
    """
    global _lfortran_runtime_library_loaded
    if not _lfortran_runtime_library_loaded:
        base_dir = get_lib_path()
        liblfort_so = os.path.join(base_dir, "liblfort.so")
        llvm.load_library_permanently(liblfort_so)
        _lfortran_runtime_library_loaded = True

def main():
    parser = argparse.ArgumentParser(description="Fortran compiler.")
    parser.add_argument('file', help="source file")
    parser.add_argument('-emit-llvm', action="store_true",
            help="emit LLVM IR source code, do not assemble or link")
    parser.add_argument('-S', action="store_true",
            help="emit assembly, do not assemble or link")
    parser.add_argument('-c', action="store_true",
            help="compile and assemble, do not link")
    parser.add_argument('-o', metavar="FILE",
            help="place the output into FILE")
    parser.add_argument('--ld-musl', action="store_true",
            help="invoke ld directly and link with musl")
    parser.add_argument('-v', action="store_true",
            help="be more verbose")
    parser.add_argument('-O3', action="store_true",
            help="turn LLVM optimizations on")
    args = parser.parse_args()

    filename = args.file
    basename, ext = os.path.splitext(os.path.basename(filename))
    link_only = (open(filename, mode="rb").read(4)[1:] == b"ELF")
    verbose = args.v
    optimize = args.O3
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
        if verbose: print("Reading...")
        source = open(filename).read()
        if verbose: print("    Done.")
        if verbose: print("Importing parser...")
        from .ast import parse, SyntaxErrorException
        if verbose: print("    Done.")
        try:
            if verbose: print("Parsing...")
            ast_tree = parse(source)
            if verbose: print("    Done.")
        except SyntaxErrorException as err:
            print(str(err))
            sys.exit(1)
        if verbose: print("Importing semantic analyzer...")
        from .semantic.analyze import create_symbol_table, annotate_tree
        if verbose: print("    Done.")
        if verbose: print("Symbol table...")
        symbol_table = create_symbol_table(ast_tree)
        if verbose: print("    Done.")
        if verbose: print("Type annotation...")
        annotate_tree(ast_tree, symbol_table)
        if verbose: print("    Done.")
        if verbose: print("Importing codegen...")
        from .codegen.gen import codegen
        if verbose: print("    Done.")
        if verbose: print("LLMV IR generation...")
        module = codegen(ast_tree, symbol_table)
        if verbose: print("    Done.")
        if verbose: print("LLMV IR -> string...")
        source_ll = str(module)
        if verbose: print("    Done.")

        # LLVM IR source -> assembly / machine object code
        if verbose: print("Initializing LLVM...")
        llvm.initialize()
        llvm.initialize_native_asmprinter()
        llvm.initialize_native_target()
        target = llvm.Target.from_triple(module.triple)
        target_machine = target.create_target_machine()
        target_machine.set_asm_verbosity(True)
        if verbose: print("    Done.")
        if verbose: print("Loading LLMV IR string...")
        mod = llvm.parse_assembly(source_ll)
        if verbose: print("    Done.")
        if verbose: print("Verifying LLMV IR...")
        mod.verify()
        if verbose: print("    Done.")
        if optimize:
            if verbose: print("Optimizing...")
            pmb = llvm.create_pass_manager_builder()
            pmb.opt_level = 3
            pmb.loop_vectorize = True
            pmb.slp_vectorize = True
            pm = llvm.create_module_pass_manager()
            pmb.populate(pm)
            pm.run(mod)
            if verbose: print("    Done.")

        if args.emit_llvm:
            with open(outfile, "w") as ll:
                ll.write(str(mod))
            return
        if args.S:
            with open(outfile, "w") as o:
                o.write(target_machine.emit_assembly(mod))
            return
        if verbose: print("Machine code...")
        with open(objfile, "wb") as o:
            o.write(target_machine.emit_object(mod))
        if verbose: print("    Done.")
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
        base_dir = get_lib_path()
        os.system("cc -o %s %s -L%s -llfort -lm" % (outfile, objfile, base_dir))
    if objfile == "tmp_object_file.o":
        os.system("rm %s" % objfile)
