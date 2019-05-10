# Fortran Language

## Background and Motivation

Fortran was designed from the ground up to naturally and simply translate
mathematics to code that compiles and runs at maximum speed. And being
specifically targeted for such fundamentally computational tasks, it contains a
broad range of key functionality within the language itself, standard across
all platforms, with no need for external libraries that may or may not be well
optimized or maintained, at present or down the road.

Some highlights:

* Multidimensional arrays which can be allocated and indexed as the
  math/science dictates (not restricted to start at 0 or 1) and can be sliced
  as desired (as, e.g., in MATLAB);
* Operators which operate naturally upon the aforementioned arrays/matrices, as
  they do scalars;
* Complex numbers;
* Special functions;
* Structures and pointers for more general data representation.

Because the essentials are contained in the language itself, it is simple to
read and write, without need of choosing from among or deciphering a
proliferation of external classes to do the same thing. And because the
essentials are self-contained, compilers can provide detailed compile-time
(e.g., argument mismatch) and run-time (e.g., memory access) checks, as well as
highly optimized executables, directly from natural, readable code without need
of extensive optimization heroics by the developer.

See our blog posts for more information:

* [Why We Created LFortran](https://lfortran.org/blog/2019/04/why-we-created-lfortran/)
* [Why to Use Fortran For New Projects](https://lfortran.org/blog/2019/05/why-to-use-fortran-for-new-projects/)

## How to Learn Fortran

Fortran is relatively quick to learn because it is so much simpler and smaller
than C/C++ (in practice, that is, with all needed libraries included).
If you are interested in learning more, please see our webpage at
[fortran90.org] with recommended practices for writing code, side by side
comparison with Python/NumPy, links to other online Fortran resources and
books, and an FAQ.

[fortran90.org]: https://www.fortran90.org/
