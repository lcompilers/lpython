# LPython Tutorial: 1

This is the first tutorial of LPython compiler.

## Installation

Please follow the steps mentioned in the
[LPython's installation instructions](https://github.com/lcompilers/lpython#installation).

After installation, verify it using:
```
% lpython --version
LPython version: 0.20.0
Platform: macOS ARM
Default target: arm64-apple-darwin22.1.0
```


## Getting started with lpython

Let's begin by compiling the very first code example using `lpython`.
Copy the contents of `hello.py`.

```
% cat hello.py
def main():
    print("Hello LPython!")

main()
```

Compile and run it using:

```
% lpython hello.py
Hello LPython!
```

`lpython` is designed to work exactly in the same way as `python`. LPython
is designed such that it is the **subset** of CPython and with the aim of
whatever works in LPython should also work in CPython!

Now, if we want to compile and run separately, we can do:

```
% lpython hello.py -o hello.out # just compile it!
% ./hello.out # run the binary!
Hello LPython!
```

### Variables

LPython is a strictly typed python compiler, meaning, each variable must
have a type just like C, or C++. Some of the basic supported types are

| Types    | Description |
| -------- | ------- |
| i1, i8, i16, i32, i64  | Integer type supporting 1, 8, 16, 32, and 64 bits |
| u8, u16, u32, u64 | Unsigned Integer type supporting 8, 16, 32, and 64 bits |
| str | String type|
| bool | Boolean type |
| f32, f64    | Real type supporting 32 and 64 bits  |
| c32, c64    | Real type supporting 32 and 64 bits |
| CPtr | C Pointer type |
| Const[T] | Constant type |
| Callable| Callable type |
| Allocatable | Allocatable type |
| Pointer | Pointer type |
| S | Symbolic type |

Try the following variable example
```
% cat var1.py
from lpython import i32, i64

def variable_function_1():
    x: i32 = 1
    y: i32 = 2
    print("x + y is", x + y)

    x1: i64 = i64(10)
    y1: i64 = i64(20)
    print("x1 + y1 is", x1 + y1)

variable_function_1()
```

Compile and run it

```
% lpython var1.py
x + y is 3
x1 + y1 is 30
```

Note that while instantiating `x1`, we have to **explicit casting** as
default integer type in LPython is `i32`. Implicit casting is
not supported to avoid any magic happening in your code and easy debugging.
Look at the nice error thrown below if we haven't added explicit cast.

```
% cat var2.py
from lpython import i32, i64

def variable_function_error_1():
    x1: i64 = 10
    y1: i64 = 20
    print("x1 + y1 is", x1 + y1)

variable_function_error_1()
```

Compile and try to run it
```
% lpython var2.py
semantic error: Type mismatch in annotation-assignment, the types must be compatible
 --> var2.py:4:5
  |
4 |     x1: i64 = 10
  |     ^^        ^^ type mismatch ('i64' and 'i32')


Note: Please report unclear or confusing messages as bugs at
https://github.com/lcompilers/lpython/issues.
```

Similarly, please note that the default type of float is `f64`.
