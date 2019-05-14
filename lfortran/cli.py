import argparse
import os
import sys

import llvmlite.binding as llvm

def get_lib_path():
    here = os.path.abspath(os.path.dirname(__file__))
    if here.startswith(sys.prefix):
        # We are installed, use the install location
        root_dir = sys.prefix
    else:
        # We run from git, use the relative location
        root_dir = os.path.abspath(os.path.join(here, ".."))
    if sys.platform == "win32":
        base_dir = os.path.join(root_dir, "bin")
    else:
        base_dir = os.path.join(root_dir, "share", "lfortran", "lib")
    if not os.path.exists(base_dir):
        raise Exception("LFortran runtime library path does not exist: %s" \
            % base_dir)
    return base_dir

_lfortran_runtime_library_loaded = False

def load_lfortran_runtime_library():
    """
    Locates and loads the LFortran runtime library.

    If the library is already loaded, it will not load it again.
    """
    global _lfortran_runtime_library_loaded
    if not _lfortran_runtime_library_loaded:
        base_dir = get_lib_path()
        if sys.platform == "linux":
            shprefix = "lib"
            shsuffix = "so"
        elif sys.platform == "win32":
            shprefix = ""
            shsuffix = "dll"
        elif sys.platform == "darwin":
            shprefix = "lib"
            shsuffix = "dylib"
        else:
            raise NotImplementedError("Platform not supported yet.")
        liblfortran_so = os.path.join(base_dir, shprefix + "lfortran." \
                + shsuffix)
        llvm.load_library_permanently(liblfortran_so)
        _lfortran_runtime_library_loaded = True

def main():
    parser = argparse.ArgumentParser(description="Fortran compiler.")
    # Standard options compatible with gfortran or clang
    # We follow the established conventions
    std = parser.add_argument_group('standard arguments',
        'compatible with other compilers')
    std.add_argument('file', help="source file", nargs="?")
    std.add_argument('-emit-llvm', action="store_true",
            help="emit LLVM IR source code, do not assemble or link")
    std.add_argument('-S', action="store_true",
            help="emit assembly, do not assemble or link")
    std.add_argument('-c', action="store_true",
            help="compile and assemble, do not link")
    std.add_argument('-o', metavar="FILE",
            help="place the output into FILE")
    std.add_argument('-v', action="store_true",
            help="be more verbose")
    std.add_argument('-O3', action="store_true",
            help="turn LLVM optimizations on")
    std.add_argument('-E', action="store_true",
            help="not used (present for compatibility with cmake)")
    # LFortran specific options, we follow the Python conventions:
    # short option (-k), long option (--long-option)
    lf = parser.add_argument_group('LFortran arguments',
        'specific to LFortran')
    lf.add_argument('--ld-musl', action="store_true",
            help="invoke ld directly and link with musl")
    lf.add_argument('--show-ast', action="store_true",
            help="show AST for the given file and exit")
    lf.add_argument('--show-asr', action="store_true",
            help="show ASR for the given file and exit")
    lf.add_argument('--show-ast-typed', action="store_true",
            help="show type annotated AST (parsing and semantics) for the given file and exit (deprecated)")
    args = parser.parse_args()

    if args.E:
        # CMake calls `lfort` with -E, and if we silently return a non-zero
        # exit code, CMake does not produce an error to the user.
        sys.exit(1)

    filename = args.file
    verbose = args.v
    optimize = args.O3

    if not filename:
        from lfortran.prompt import main
        main(verbose=verbose)
        return

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
        if verbose: print("Reading...")
        source = open(filename).read()
        if verbose: print("    Done.")
        if len(sys.argv) == 2 and filename:
            # Run as a script:
            if verbose: print("Importing evaluator...")
            from lfortran.codegen.evaluator import FortranEvaluator
            if verbose: print("    Done.")
            e = FortranEvaluator()
            e.evaluate(source)
            return
        if verbose: print("Importing parser...")
        from .ast import (src_to_ast, SyntaxErrorException, print_tree,
                print_tree_typed)
        if verbose: print("    Done.")
        try:
            if verbose: print("Parsing...")
            if args.show_ast or args.show_ast_typed or args.show_asr:
                ast_tree = src_to_ast(source, translation_unit=False)
            else:
                ast_tree = src_to_ast(source)
            if verbose: print("    Done.")
        except SyntaxErrorException as err:
            print(str(err))
            sys.exit(1)
        if args.show_ast:
            print_tree(ast_tree)
            return
        if args.show_asr:
            from lfortran import ast_to_asr
            from lfortran.asr.pprint import pprint_asr
            asr_repr = ast_to_asr(ast_tree)
            pprint_asr(asr_repr)
            return
        if verbose: print("Importing semantic analyzer...")
        from .semantic.analyze import create_symbol_table, annotate_tree
        if verbose: print("    Done.")
        if verbose: print("Symbol table...")
        symbol_table = create_symbol_table(ast_tree)
        if verbose: print("    Done.")
        if verbose: print("Type annotation...")
        annotate_tree(ast_tree, symbol_table)
        if verbose: print("    Done.")
        if args.show_ast_typed:
            print_tree_typed(ast_tree)
            return
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
        os.system("cc -o %s %s -L%s -Wl,-rpath=%s -llfortran -lm" % (outfile,
            objfile, base_dir, base_dir))
    if objfile == "tmp_object_file.o":
        os.system("rm %s" % objfile)
