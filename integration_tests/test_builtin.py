from lpython import i32

def test_ord():
    s: str

    exclamation_unicode: i32
    s = '!'
    exclamation_unicode = ord(s)
    assert(ord('!') == 33) # testing compile time implementation
    assert(exclamation_unicode == 33) # testing runtime implementation

    dollar_unicode: i32
    s = '$'
    dollar_unicode = ord(s)
    assert(ord('$') == 36) # testing compile time implementation
    assert(dollar_unicode == 36) # testing runtime implementation

    left_parenthesis_unicode: i32
    s = '('
    left_parenthesis_unicode = ord(s)
    assert(ord('(') == 40) # testing compile time implementation
    assert(left_parenthesis_unicode == 40) # testing runtime implementation

    plus_unicode: i32
    s = '+'
    plus_unicode = ord(s)
    assert(ord('+') == 43) # testing compile time implementation
    assert(plus_unicode == 43) # testing runtime implementation

    zero_unicode: i32
    s = '0'
    zero_unicode = ord(s)
    assert(ord('0') == 48) # testing compile time implementation
    assert(zero_unicode == 48) # testing runtime implementation

    nine_unicode: i32
    s = '9'
    nine_unicode = ord(s)
    assert(ord('9') == 57) # testing compile time implementation
    assert(nine_unicode == 57) # testing runtime implementation

    semicolon_unicode: i32
    s = ';'
    semicolon_unicode = ord(s)
    assert(ord(';') == 59) # testing compile time implementation
    assert(semicolon_unicode == 59) # testing runtime implementation

    capital_a_unicode: i32
    s = 'A'
    capital_a_unicode = ord(s)
    assert(ord('A') == 65) # testing compile time implementation
    assert(capital_a_unicode == 65) # testing runtime implementation

    capital_z_unicode: i32
    s = 'Z'
    capital_z_unicode = ord(s)
    assert(ord('Z') == 90) # testing compile time implementation
    assert(capital_z_unicode == 90) # testing runtime implementation

    right_bracket_unicode: i32
    s = ']'
    right_bracket_unicode = ord(s)
    assert(ord(']') == 93) # testing compile time implementation
    assert(right_bracket_unicode == 93) # testing runtime implementation

    small_a_unicode: i32
    s = 'a'
    small_a_unicode = ord(s)
    assert(ord('a') == 97) # testing compile time implementation
    assert(small_a_unicode == 97) # testing runtime implementation

    small_z_unicode: i32
    s = 'z'
    small_z_unicode = ord(s)
    assert(ord('z') == 122) # testing compile time implementation
    assert(small_z_unicode == 122) # testing runtime implementation

    right_brace_unicode: i32
    s = '}'
    right_brace_unicode = ord(s)
    assert(ord('}') == 125) # testing compile time implementation
    assert(right_brace_unicode == 125) # testing runtime implementation


def test_chr():
    i: i32
   
    exclamation: str
    i = 33
    exclamation = chr(i)
    assert(chr(33) == '!') # testing compile time implementation
    assert(exclamation == '!') # testing runtime implementation

    dollar: str
    i = 36
    dollar = chr(i)
    assert(chr(36) == '$') # testing compile time implementation
    assert(dollar == '$') # testing runtime implementation

    left_parenthesis: str
    i = 40
    left_parenthesis = chr(i)
    assert(chr(40) == '(') # testing compile time implementation
    assert(left_parenthesis == '(') # testing runtime implementation

    plus: str
    i = 43
    plus = chr(i)
    assert(chr(43) == '+') # testing compile time implementation
    assert(plus == '+') # testing runtime implementation

    zero: str
    i = 48
    zero = chr(i)
    assert(chr(48) == '0') # testing compile time implementation
    assert(zero == '0') # testing runtime implementation

    nine: str
    i = 57
    nine = chr(i)
    assert(chr(57) == '9') # testing compile time implementation
    assert(nine == '9') # testing runtime implementation

    semicolon: str
    i = 59
    semicolon = chr(i)
    assert(chr(59) == ';') # testing compile time implementation
    assert(semicolon == ';') # testing runtime implementation

    capital_a: str
    i = 65
    capital_a = chr(i)
    assert(chr(65) == 'A') # testing compile time implementation
    assert(capital_a == 'A') # testing runtime implementation

    capital_z: str
    i = 90
    capital_z = chr(i)
    assert(chr(90) == 'Z') # testing compile time implementation
    assert(capital_z == 'Z') # testing runtime implementation

    right_bracket: str
    i = 93
    right_bracket = chr(i)
    assert(chr(93) == ']') # testing compile time implementation
    assert(right_bracket == ']') # testing runtime implementation

    small_a: str
    i = 97
    small_a = chr(i)
    assert(chr(97) == 'a') # testing compile time implementation
    assert(small_a == 'a') # testing runtime implementation

    small_z: str
    i = 122
    small_z = chr(i)
    assert(chr(122) == 'z') # testing compile time implementation
    assert(small_z == 'z') # testing runtime implementation

    right_brace: str
    i = 125
    right_brace = chr(i)
    assert(chr(125) == '}') # testing compile time implementation
    assert(right_brace == '}') # testing runtime implementation

def more_test():
    p: i32
    q: i32
    r: i32
    s: i32
    p = 97 # char 'a'
    q = 112 # char 'p'
    r = 10 # newline char
    s = 65 # char 'A'

    print(chr(p))
    print(chr(q))
    print(chr(r))
    print(chr(s))

    a: str
    b: str
    c: str
    d: str
    a = "!"
    b = " "
    c = "Z"
    d = "g"
    print(ord(a))
    print(ord(b))
    print(ord(c))
    print(ord(d))

test_ord()
test_chr()
more_test()
