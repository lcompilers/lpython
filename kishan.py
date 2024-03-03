def is_alpha():
    a: str = "helloworld"
    b: str = "hj kl"
    c: str = "a12(){}A"
    d: str = " "
    e: str = ""
    f: str = "ab23"
    g: str = "ab2%3"
    res: bool = a.isalnum()
    res2: bool = b.isalnum()
    res3: bool = c.isalnum()
    res4: bool = d.isalnum()
    res5: bool = e.isalnum()
    res6: bool = f.isalnum()
    res7: bool = g.isalnum()

    assert res == True
    assert res2 == False
    assert res3 == False
    assert res4 == False
    assert res5 == False
    assert res6 == True
    assert res7 == False

    assert "helloworld".isalnum() == True
    assert "hj kl".isalnum() == False
    assert "a12(){}A".isalnum() == False
    assert " ".isalnum() == False
    assert "".isalnum() == False
    assert "ab23".isalnum() == True
    assert "ab2%3".isalnum() == False

is_alpha()
