def testEndSepKeywords():
    print("abc", "lmn", "pqr")
    print("abc", "lmn", "pqr", sep = "+")
    print("abc", "lmn", "pqr", end = "xyz\n")
    print("abc", "lmn", "pqr", sep = "+", end = "xyz\n")

testEndSepKeywords()