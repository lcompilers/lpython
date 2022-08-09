def test_frompyfunc_name(self):
    # name conversion was failing for python 3 strings
    # resulting in the default '?' name. Also test utf-8
    # encoding using non-ascii name.
    def cassé(x):
        return x

ℙƴtℌøἤ = 1
print(ℙƴtℌøἤ)

if 诶 != 2:
    pass

def क१():
    ...
