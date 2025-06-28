# lpython

https://lpython.org/

This is a python package to add typing information to python code. Use LPython
to compile it.

## Example

```python
from lpython import i32, f64

def add(a: i32, b: i32) -> i32:
    return a + b

def area_of_circle(radius: f64) -> f64:
    pi: f64 = 3.14
    return pi * (radius * radius)

def main0():
    print("The sum of 5 and 3 is", end=" ")
    print(add(5, 3))
    print("Area of circle with radius 5.0 is", end=" ")
    print(area_of_circle(5.0))

main0()
```

```bash
$ python main.py
The sum of 5 and 3 is 8
Area of circle with radius 5.0 is 78.5
```
