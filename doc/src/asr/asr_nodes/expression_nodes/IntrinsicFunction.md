# IntrinsicFunction

An intrinsic function. An **expr** node.

## Declaration

### Syntax

```
IntrinsicFunction(expr* args, int intrinsic_id, int overload_id,
    ttype type, expr? value)
```

### Arguments

* `args` represents all arguments passed to the function
* `intrinsic_id` is the unique ID of the generic intrinsic function
* `overload_id` is the ID of the signature within the given generic function
* `type` represents the type of the output
* `value` is an optional compile time value

### Return values

The return value is the expression that the `IntrinsicFunction` represents.

## Description

**IntrinsicFunction** represents an intrinsic function (such as `Abs`,
`Modulo`, `Sin`, `Cos`, `LegendreP`, `FlipSign`, ...) that either the backend
or the middle-end (optimizer) needs to have some special logic for. Typically a
math function, but does not have to be.

IntrinsicFunction is both side-effect-free (no writes to global variables) and
deterministic (no reads from global variables). They are also elemental: can be
vectorized over any argument(s). They can be used inside parallel code and
cached.

The `intrinsic_id` determines the generic function uniquely (`Sin` and `Abs`
have different number, but `IntegerAbs` and `RealAbs` share the number) and
`overload_id` uniquely determines the signature starting from 0 for each
generic function (e.g., `IntegerAbs`, `RealAbs` and `ComplexAbs` can have
`overload_id` equal to 0, 1 and 2, and `RealSin`, `ComplexSin` can be 0, 1).

Backend use cases: Some architectures have special hardware instructions for
operations like Sqrt or Sin and if they are faster than a software
implementation, the backend will use it. This includes the `FlipSign` function
which is our own "special function" that the optimizer emits for certain
conditional floating point operations, and the backend emits an efficient bit
manipulation implementation for architectures that support it.

Middle-end use cases: the middle-end can use the high level semantics to
simplify, such as `sin(e)**2 + cos(e)**2 -> 1`, or it could approximate
expressions like `if (abs(sin(x) - 0.5) < 0.3)` with a lower accuracy version
of `sin`.

We provide ASR -> ASR lowering transformations that substitute the given
intrinsic function with an ASR implementation using more primitive ASR nodes,
typically implemented in the surface language (say a `sin` implementation using
argument reduction and a polynomial fit, or a `sqrt` implementation using a
general power formula `x**(0.5)`, or `LegendreP(2,x)` implementation using a
formula `(3*x**2-1)/2`).

This design also makes it possible to allow selecting using command line
options how certain intrinsic functions should be implemented, for example if
trigonometric functions should be implemented using our own fast
implementation, `libm` accurate implementation, we could also call into other
libraries. These choices should happen at the ASR level, and then the result
further optimized (such as inlined) as needed.

## Types

The argument types in `args` have the types of the corresponding signature as
determined by `intrinsic_id`. For example `IntegerAbs` accepts an integer, but
`RealAbs` accepts a real.

## Examples

The following example code creates `IntrinsicFunction` ASR node:

```fortran
sin(0.5)
```

ASR:

```
(TranslationUnit
    (SymbolTable
        1
        {
        })
    [(IntrinsicFunction
        [(RealConstant
            0.500000
            (Real 4 [])
        )]
        0
        0
        (Real 4 [])
        (RealConstant 0.479426 (Real 4 []))
    )]
)
```

## See Also

[FunctionCall]()