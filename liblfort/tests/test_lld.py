from subprocess import call

import llvmlite.binding as llvm

def test_linux64_program():
    """
    This is the simplest assembly program that uses Linux 64bit syscall to
    return an exit value.
    """
    # Named registers are used in the "asm" block, so that LLVM handles
    # register allocation. The syscall kernel call clobbers rcx and r11
    # registers. The LLVM's "asm" call itself seems to clobber dirflag, fpsr
    # and flags registers.
    source_ll = r"""
; exit(int exit_code)
define void @exit(i64 %exit_code) {
    call i64 asm sideeffect "syscall",
        "={rax},{rax},{rdi},~{rcx},~{r11},~{dirflag},~{fpsr},~{flags}"
        ( i64 60          ; {rax} SYSCALL_EXIT
        , i64 %exit_code  ; {rdi} exit_code
        )
    ret void
}

define void @_start() {
    call void @exit(i64 42)
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

def test_linux64_program_write(capfd):
    source_ll = r"""
@.message = internal constant [14 x i8] c"Hello, world!\0A"

; write(STDOUT, message, message_len)
define void @write(i64 %STDOUT, i8* %message, i64 %message_len) {
    call i64 asm sideeffect "syscall",
        "={rax},{rax},{rdi},{rsi},{rdx},~{rcx},~{r11},~{dirflag},~{fpsr},~{flags}"
        ( i64 1            ; {rax} SYSCALL_WRITE
        , i64 %STDOUT      ; {rdi} STDOUT
        , i8* %message     ; {rsi} message
        , i64 %message_len ; {rdx} message_len
        )
    ret void
}

; exit(int exit_code)
define void @exit(i64 %exit_code) {
    call i64 asm sideeffect "syscall",
        "={rax},{rax},{rdi},~{rcx},~{r11},~{dirflag},~{fpsr},~{flags}"
        ( i64 60          ; {rax} SYSCALL_EXIT
        , i64 %exit_code  ; {rdi} exit_code
        )
    ret void
}

define void @_start() {
    %message_ptr = getelementptr [14 x i8], [14 x i8]* @.message , i64 0, i64 0

    call void @write(i64 1, i8* %message_ptr, i64 14)
    call void @exit(i64 42)
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

    out = capfd.readouterr().out
    assert out == "Hello, world!\n"