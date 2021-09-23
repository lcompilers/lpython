# LFortran Development Status

This page documents what Fortran features are supported by LFortran. For each
feature we list a short description, the filename with the test and current
status for each parts of the compiler:

* AST: The code can be parsed to AST (`lfortran --show-ast test.f90`)
* ASR: The code can be transformed to ASR (`lfortran --show-asr test.f90`)
* LLVM: LFortran can generate LLVM IR (`lfortran --show-llvm test.f90`)
* BIN: The LLVM IR can compile to a binary
* RUN: The binary runs without errors

If all are green it means the feature fully works and you can use it in your
codes. Otherwise you can see what the status is of each feature.

This page is generated automatically using the [Compiler
Tester](https://gitlab.com/lfortran/compiler_tester) repository which contains
all the Fortran tests and scripts to run LFortran to produce the tables below.
We are looking for contributors to contribute more tests. Our goal is to have a
comprehensive Fortran testsuite that can be used to test any Fortran compiler.


Testing the LFortran compiler version:

```console
$ lfortran --version
LFortran version: 0.12.0-491-gaf48ff273
Platform: macOS
Default target: x86_64-apple-darwin20.3.0
```

# Topics

## Full programs that compute something interesting

### Basic Numerics

Directory: `tests/programs/numerics`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `Pure Fortran sin(x) implementation` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_sin_implementation.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/programs/numerics/t01_sin_implementation.f90) |


## Modules

### Basic Usage

Directory: `tests/modules/basic`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `Basic modules` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/modules/basic/t01.f90) |


### Module Functions and Subroutines

Directory: `tests/modules/procedures`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `Module functions` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/modules/procedures/t01.f90) |
| `Module subroutines` | ✅  | ✅  | ✅   | ✅  | ✅  | [t02.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/modules/procedures/t02.f90) |
| `Nested subroutines` | ✅  | ✅  | ✅   | ✅  | ✅  | [t03.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/modules/procedures/t03.f90) |


## Expressions

### Arithmetic Operations

Directory: `tests/expressions/arit`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `+,-,*,/,**` | ✅  | ✅  | ✅   | ✅  | ✅  | [basic_operations.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/expressions/arit/basic_operations.f90) |


### Integers

Directory: `tests/expressions/integers`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `integers` | ✅  | ✅  | ✅   | ✅  | ✅  | [integer_kind.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/expressions/integers/integer_kind.f90) |
| `relational operations` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_rel_operations.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/expressions/integers/t01_rel_operations.f90) |
| `logical operations` | ✅  | ✅  | ✅   | ✅  | ✅  | [t02_logical_operations.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/expressions/integers/t02_logical_operations.f90) |


### Real Numbers

Directory: `tests/expressions/reals`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `single/double reals` | ✅  | ✅  | ✅   | ✅  | ✅  | [real_kind.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/expressions/reals/real_kind.f90) |
| `defined operator` | ✅  | ❌  | ❌   | ❌  | ❌  | [t01_def_op.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/expressions/reals/t01_def_op.f90) |


### Complex Numbers

Directory: `tests/expressions/complex`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `single/double complex` | ✅  | ✅  | ✅   | ✅  | ✅  | [complex_kind.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/expressions/complex/complex_kind.f90) |


### Strings

Directory: `tests/expressions/character`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `character` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_character.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/expressions/character/t01_character.f90) |
| `string concatenation` | ✅  | ✅  | ✅   | ✅  | ✅  | [t02_concat_operation.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/expressions/character/t02_concat_operation.f90) |


### Derived Types

Directory: `tests/expressions/derived_type`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `basic derived types` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_derived_type.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/expressions/derived_type/t01_derived_type.f90) |


## Statements

### Allocate Statement

Directory: `tests/statements/allocate`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `allocate statement` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/statements/allocate/t01.f90) |


### Block Statement

Directory: `tests/statements/block`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `block statement` | ✅  | ❌  | ❌   | ❌  | ❌  | [t01.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/statements/block/t01.f90) |


### Goto Statement

Directory: `tests/statements/goto`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `computed go-to statement` | ✅  | ❌  | ❌   | ❌  | ❌  | [t01.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/statements/goto/t01.f90) |
| `go-to statement` | ✅  | ❌  | ❌   | ❌  | ❌  | [t02.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/statements/goto/t02.f90) |


### If Statement

Directory: `tests/statements/if`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `Test .false.` | ✅  | ✅  | ✅   | ✅  | ✅  | [if_01.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/statements/if/if_01.f90) |
| `single line if statement` | ✅  | ✅  | ✅   | ✅  | ✅  | [if_02.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/statements/if/if_02.f90) |
| `multi line if statement` | ✅  | ✅  | ✅   | ✅  | ✅  | [if_03.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/statements/if/if_03.f90) |
| `nested if statements` | ✅  | ✅  | ✅   | ✅  | ✅  | [if_04.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/statements/if/if_04.f90) |


### While Statement

Directory: `tests/statements/while`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `Simple while loops` | ✅  | ✅  | ✅   | ✅  | ✅  | [while_01.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/statements/while/while_01.f90) |
| `exit / cycle in while loops` | ✅  | ✅  | ✅   | ✅  | ✅  | [while_02.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/statements/while/while_02.f90) |


### Print Statement

Directory: `tests/statements/print`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `Basic print` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/statements/print/t01.f90) |


### Open, Read, Write, Close Statement

Directory: `tests/statements/file_io`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `open/read/write/close` | ✅  | ✅  | ❌   | ❌  | ❌  | [t01.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/statements/file_io/t01.f90) |


## Intrinsic Functions

### abs

Directory: `tests/intrinsic/abs`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/abs/t01_scalar_real.f90) |
| `scalar single/double complex` | ✅  | ❌  | ❌   | ❌  | ❌  | [t02_scalar_complex.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/abs/t02_scalar_complex.f90) |
| `array 1D single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/abs/t03_array1d_real.f90) |


### exp

Directory: `tests/intrinsic/exp`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/exp/t01_scalar_real.f90) |
| `scalar single/double complex` | ✅  | ✅  | ✅   | ✅  | ✅  | [t02_scalar_complex.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/exp/t02_scalar_complex.f90) |
| `array 1D single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/exp/t03_array1d_real.f90) |


### log

Directory: `tests/intrinsic/log`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/log/t01_scalar_real.f90) |
| `scalar single/double complex` | ✅  | ✅  | ✅   | ✅  | ✅  | [t02_scalar_complex.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/log/t02_scalar_complex.f90) |
| `array 1D single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/log/t03_array1d_real.f90) |


### sqrt

Directory: `tests/intrinsic/sqrt`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/sqrt/t01_scalar_real.f90) |
| `scalar single/double complex` | ✅  | ❌  | ❌   | ❌  | ❌  | [t02_scalar_complex.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/sqrt/t02_scalar_complex.f90) |
| `array 1D single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/sqrt/t03_array1d_real.f90) |


### sin

Directory: `tests/intrinsic/sin`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/sin/t01_scalar_real.f90) |
| `scalar single/double complex` | ✅  | ❌  | ❌   | ❌  | ❌  | [t02_scalar_complex.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/sin/t02_scalar_complex.f90) |
| `array 1D single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/sin/t03_array1d_real.f90) |


### cos

Directory: `tests/intrinsic/cos`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/cos/t01_scalar_real.f90) |
| `scalar single/double complex` | ✅  | ✅  | ✅   | ✅  | ✅  | [t02_scalar_complex.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/cos/t02_scalar_complex.f90) |
| `array 1D single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/cos/t03_array1d_real.f90) |


### tan

Directory: `tests/intrinsic/tan`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/tan/t01_scalar_real.f90) |
| `scalar single/double complex` | ✅  | ✅  | ✅   | ✅  | ✅  | [t02_scalar_complex.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/tan/t02_scalar_complex.f90) |
| `array 1D single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/tan/t03_array1d_real.f90) |


### sinh

Directory: `tests/intrinsic/sinh`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/sinh/t01_scalar_real.f90) |
| `scalar single/double complex` | ✅  | ✅  | ✅   | ✅  | ✅  | [t02_scalar_complex.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/sinh/t02_scalar_complex.f90) |
| `array 1D single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/sinh/t03_array1d_real.f90) |


### cosh

Directory: `tests/intrinsic/cosh`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/cosh/t01_scalar_real.f90) |
| `scalar single/double complex` | ✅  | ✅  | ✅   | ✅  | ✅  | [t02_scalar_complex.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/cosh/t02_scalar_complex.f90) |
| `array 1D single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/cosh/t03_array1d_real.f90) |


### tanh

Directory: `tests/intrinsic/tanh`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/tanh/t01_scalar_real.f90) |
| `scalar single/double complex` | ✅  | ✅  | ✅   | ✅  | ✅  | [t02_scalar_complex.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/tanh/t02_scalar_complex.f90) |
| `array 1D single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/tanh/t03_array1d_real.f90) |


### asin

Directory: `tests/intrinsic/asin`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/asin/t01_scalar_real.f90) |
| `scalar single/double complex` | ✅  | ✅  | ✅   | ✅  | ✅  | [t02_scalar_complex.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/asin/t02_scalar_complex.f90) |
| `array 1D single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/asin/t03_array1d_real.f90) |


### acos

Directory: `tests/intrinsic/acos`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/acos/t01_scalar_real.f90) |
| `scalar single/double complex` | ✅  | ✅  | ✅   | ✅  | ✅  | [t02_scalar_complex.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/acos/t02_scalar_complex.f90) |
| `array 1D single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/acos/t03_array1d_real.f90) |


### atan

Directory: `tests/intrinsic/atan`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/atan/t01_scalar_real.f90) |
| `scalar single/double complex` | ✅  | ✅  | ✅   | ✅  | ✅  | [t02_scalar_complex.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/atan/t02_scalar_complex.f90) |
| `array 1D single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/atan/t03_array1d_real.f90) |


### modulo

Directory: `tests/intrinsic/modulo`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ❌  | ❌   | ❌  | ❌  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/modulo/t01_scalar_real.f90) |
| `array 1D single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/modulo/t03_array1d_real.f90) |


### mod

Directory: `tests/intrinsic/mod`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ❌  | ❌   | ❌  | ❌  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/mod/t01_scalar_real.f90) |
| `array 1D single/double real` | ✅  | ❌  | ❌   | ❌  | ❌  | [t03_array1d_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/mod/t03_array1d_real.f90) |


### min

Directory: `tests/intrinsic/min`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/min/t01_scalar_real.f90) |


### max

Directory: `tests/intrinsic/max`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ❌   | ❌  | ❌  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/max/t01_scalar_real.f90) |


### int

Directory: `tests/intrinsic/int`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ❌  | ❌   | ❌  | ❌  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/int/t01_scalar_real.f90) |


### real

Directory: `tests/intrinsic/real`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ✅  | ✅   | ✅  | ✅  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/real/t01_scalar_real.f90) |


### floor

Directory: `tests/intrinsic/floor`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ❌  | ❌   | ❌  | ❌  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/floor/t01_scalar_real.f90) |


### nint

Directory: `tests/intrinsic/nint`

| Description | AST | ASR | LLVM | BIN | RUN | Filename |
| ----------- | --- | --- | ---- | --- | --- | -------- |
| `scalar single/double real` | ✅  | ❌  | ❌   | ❌  | ❌  | [t01_scalar_real.f90](https://gitlab.com/lfortran/compiler_tester/-/blob/master/tests/intrinsic/nint/t01_scalar_real.f90) |


