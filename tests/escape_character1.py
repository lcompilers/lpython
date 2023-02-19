def testEscapeCharacters():
    s1: str = 'Hello\tWorld'
    s2: str = 'Hello    \bWorld'
    s3: str = 'Hello\vWorld'
    print(s1)
    print(s2)
    print(s3)

def main0():
   testEscapeCharacters()

main0()