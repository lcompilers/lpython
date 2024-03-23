# Built-in Functions

LPython has a variety of functions and types built into it that are always available.

### abs(x)

- **Parameter**
    - x : integer (i8, i16, i32, i64), floating point number (f32, f64), complex number (c32, c64) or bool
- **Returns** : integer (i8, i16, i32, i64), floating point number (f32, f64)

Returns the absolute value of a number. If the argument is a complex number, its magnitude is returned.


### bin(n)

- **Parameters**
    - n : integer (i32)
- **Returns** : str

Returns the binary representation of n as a '0b' prefixed string.


### complex(x=0, y=0)

- **Parameters**
    - x : integer (i32, i64) or floating point number (f32, f64)
    - y : integer (i32, i64) or floating point number (f32, f64)
- **Returns** : complex number (c32, c64)

Returns a complex number with the provided real and imaginary parts. Both x and y should be of the same type. However, using both the 32-bit and 64-bit versions of the same type together is allowed. In that case, the returned complex number is of 64-bit type.

Example:

```python
real: i32 = 10
imag: i64 = 22
c: c64 = complex(real, imag)
```

### divmod(x, y)

- **Parameters**
    - x : integer (i32)
    - y : integer (i32)
- **Returns** : tuple[i32, i32]

Returns the tuple (x // y, x % y).


### exp(x)

- ****Parameter****
    - x : floating point number (f32, f64)
- **Returns** : floating point number (f32, f64) between [0.0, inf]

Returns the base-e exponential of x (e<sup>x</sup>), where e is the Euler's number (2.71828). For a very large output, the function returns **inf** indicating overflow.


### hex(n)

- **Parameters**
    - n : integer (i32)
- **Returns** : str

Returns the hexadecimal representation of n as a '0x' prefixed string.


### len(s)

- **Parameters**
    - s : sequence (such as string, tuple, list or range) or collection (such as a dictionary or set)
- **Returns** : integer (i32)

Returns the number of items present in an object.


### max(x, y)

- **Parameters**
    - x : integer (i32) or floating point number (f64)
    - y : integer (i32) or floating point number (f64)
- **Returns** : integer (i32) or floating point number (f64)

Returns the greater value between x and y. Both x and y should be of the same type.


### min(x, y)

- **Parameters**
    - x : integer (i32) or floating point number (f64)
    - y : integer (i32) or floating point number (f64)
- **Returns** : integer (i32) or floating point number (f64)

Returns the smaller value between x and y. Both x and y should be of the same type.


### mod(x, y)

- **Parameters**
    - x : integer (i32, i64) or floating point number (f32, f64)
    - y : integer (i32, i64) or floating point number (f32, f64)
- **Returns** : integer (i32, i64) or floating point number (f32, f64)

Returns the remainder of x / y, or x when x is smaller than y. Both x and y should be of the same type.


### pow(x, y)

- **Parameters**
    - x : integer (i32, i64), floating point number (f32, f64), complex number (c32) or bool
    - y: integer (i32, i64), floating point number (f32, f64) or bool
- **Returns** : integer (i32), floating point number (f32, f64) or a complex number

Returns x<sup>y</sup>. When x is of type bool, y must also be of the same type. If x is 32-bit complex number (c32), y can only be a 32-bit integer (i32).

**Note** : `x ** y` is the recommended method for doing the above calculation.


### round(x)

- **Parameters**
    - x : integer (i8, i16, i32, i64), floating point number (f32, f64) or bool
- **Returns** : integer (i8, i16, i32, i64)

Returns the integer nearest to x.


### sum(arr)

- **Parameters**
    - arr : list of integers (list[i32], list[i64]) or floating point numbers (list[i32], list[f64])
- **Returns** : integer (i32, i64) or floating point number (f32, f64)

Returns the sum of all elements present in the list.


### oct(n)

- **Parameters**
    - n : integer (i32)
- **Returns** : str

Returns the octal representation of n as a '0o' prefixed string.




