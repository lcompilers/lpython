F"Hello, {first_name} {last_name}. You are {age} years old."
print(f"{__file__} executed in {elapsed} seconds.")

"\nsometext\nanotherline\n"

# TODO: Support format specifiers
# print(f"{__file__} executed in {elapsed:0.2f} seconds.")

b"Some text goes here."
B"""
Multiple texts goes here.
"""

r"""
Text
"""
R"Text"
r'a\tb\nA\tB'

u"1,2,3 # comment"
u'Text'
U"Text"

fr"""Text {id}"""
Fr"Text {id}"
fR"Text {id}"
FR"Text {id}"
rf"Text {id}"
rF"""
Text {id}
"""
Rf'Text {id}'
RF"Text {id}"

br"Text"
Br"""
Text
"""
bR"Text"

BR"Text"
rb"Text"
rB"""
Text
"""
Rb'Text'
RB"Text"

rf'\N{AMPERSAND}'

(f"Text{a}, {b}"
f"Text {a}")

(f"Text{a}, {b}"
"Text")

("Text"
f"{b}, Text")

(f"Text {a}"
r"Text")

(r"Text"
r"Text")

(r"Text"
"Text")

(r"Text"
f"{a} Text")

(b"Text"
b"Text")

r'\n'
r"""
Text
123\n\t
"""
rb'\n'
rf'\n'

b'\n'
b'''
\n\\n'''
