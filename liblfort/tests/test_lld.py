from subprocess import call

import llvmlite.binding as llvm

def test_linux64_program():
    """
    This is the simplest assembly program that uses Linux 64bit syscall to
    return an exit value.
    """
    source_ll = r"""
define void @syscall1(i64 %n, i64 %a) {
    call void asm sideeffect "movq $0, %rax\0Amovq $1, %rdi\0Asyscall", "m,m,~{rax},~{rdi},~{dirflag},~{fpsr},~{flags}"(i64 %n, i64 %a)
    ret void
}

define void @_start() {
    call void @syscall1(i64 60, i64 42)
    ret void
}
    """
    objfile = "test1.o"
    binfile = "test1"

    llvm.initialize()
    llvm.initialize_native_asmprinter()
    llvm.initialize_native_asmparser()
    llvm.initialize_native_target()
    target = llvm.Target.from_triple(llvm.get_default_triple())
    target_machine = target.create_target_machine()
    mod = llvm.parse_assembly(source_ll)
    mod.verify()
    with open(objfile, "wb") as o:
        o.write(target_machine.emit_object(mod))
    llvm.lld_main(["ld.lld", "-o", binfile, objfile])
    r = call("./%s" % binfile)
    assert r == 42