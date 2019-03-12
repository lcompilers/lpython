"""
# ASR Check

This goes over the whole ASR and checks that all requirements are met:

* It is a valid Fortran code
* All additional internal consistency requirements are satisfied

This is not meant to report nice user errors, this is only meant to be run in
Debug mode to ensure that LFortran always constructs ASR in the correct form.

If one *knows* (by checking in Debug mode) that a given algorithm constructs
ASR in the correct form, then one can construct ASR directly using the
classes in the `asr.asr` module. Otherwise one should use the `asr.builder`
module, which will always construct ASR in the correct form, or report a nice
error (that can then be forwarded to the user by LFortran) even in both Debug
and Release modes. The `asr.builder` is built to be robust and handle any
(valid or invalid) input.

The semantic phase then traverses the AST and uses `asr.builder` to construct
ASR. Thus the `asr.builder` does most of the semantic checks for the semantic
analyzer (which only forwards the errors to the user), thus greatly simplifying
the semantic part of the compiler.

The hard work of doing semantic checks is encoded in the ASR module, which
does not depend on the rest of LFortran and can be used, verified and
improved independently.
"""

# TODO: Make this a visitor

def check_function(f):
    for arg in f.args:
        assert arg.name in f.symtab.symbols
        assert arg.dummy == True
    assert f.return_var.name in f.symtab.symbols
    assert f.return_var.dummy == True
    assert f.return_var.intent is None