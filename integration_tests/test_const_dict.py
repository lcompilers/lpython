from lpython import i32, f64, Const

CONST_DICTIONARY_INTEGR: Const[dict[str, i32]] = {"a": 1, "b": 2, "c": 3}

print(CONST_DICTIONARY_INTEGR.get("a"))
assert CONST_DICTIONARY_INTEGR.get("a") == 1

print(CONST_DICTIONARY_INTEGR.keys())
assert len(CONST_DICTIONARY_INTEGR.keys()) == 3

print(CONST_DICTIONARY_INTEGR.values())
assert len(CONST_DICTIONARY_INTEGR.values()) == 3

CONST_DICTIONARY_FLOAT: Const[dict[str, f64]] = {"a": 1.0, "b": 2.0, "c": 3.0}

print(CONST_DICTIONARY_FLOAT.get("a"))
assert CONST_DICTIONARY_FLOAT.get("a") == 1.0

print(CONST_DICTIONARY_FLOAT.keys())
assert len(CONST_DICTIONARY_FLOAT.keys()) == 3

print(CONST_DICTIONARY_FLOAT.values())
assert len(CONST_DICTIONARY_FLOAT.values()) == 3

