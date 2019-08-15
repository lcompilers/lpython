add_library(doctest STATIC doctest.cpp)
add_library(p::doctest ALIAS doctest)
