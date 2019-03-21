# Fortran Language

## Background and Motivation

Fortran is built from the ground up to translate mathematics into simple,
readable, and fast code – straightforwardly maintainable by the gamut of
mathematicians, scientists, and engineers who actually produce/apply that
mathematics. If that is the task at hand, it is the right tool for the job and,
as right tools tend to do, can save enormous time and pain, with most excellent
results. If, however, mathematics is not the main task, then almost certainly
C/C++, or a host of other more general-purpose languages, will be much better.

An important point is that having all the basic elements in the language itself
greatly simplifies both writing and reading fast, robust code. Writing and
reading is simplified because there is a single standard set of
functions/constructs. No need to wonder what class the array operations are
coming from, what the member functions are, how well it will be maintained in 5
years, how well optimized it is/will be, etc. All the basics are in the
language itself: fast, robust, and standard for all to read and write – and for
the compiler writers to optimize at the machine level.

## How to Learn Fortran

Fortran is relatively quick to learn because it is so much simpler and smaller
than C/C++ (in practice, that is, with all needed libraries included).
A good online resource to learn modern Fortran is:

* [fortran90.org](https://www.fortran90.org)

The website provides the
[recommended practices](https://www.fortran90.org/src/best-practices.html)
for modern Fortran and also has
[side-by-side examples](https://www.fortran90.org/src/rosetta.html)
of using Fortran and Python for common numerical tasks,
highliting the conceptual similarities between the two languages.
The website also maintains a
[list of books](https://www.fortran90.org/src/faq.html#what-are-good-books-to-learn-fortran-from)
and links to
[other online resources](https://www.fortran90.org/src/faq.html#what-are-other-good-online-fortran-resources)
about Fortran.
