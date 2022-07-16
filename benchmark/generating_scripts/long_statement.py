#!/usr/bin/python
N = 100
s1 = "(a*z+3+2*x + 3*y - x/(z**2-4) - x**(y**z))"
s = s1
for n in range(N):
    s = s + " * " + s1
print(s)
