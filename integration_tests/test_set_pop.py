def set_pop_str():
    s: set[str] = {'a', 'b', 'c'}

    assert s.pop() in {'a', 'b', 'c'}
    assert len(s) == 2
    assert s.pop() in {'a', 'b', 'c'}
    assert s.pop() in {'a', 'b', 'c'}
    assert len(s) == 0

    s.add('d')
    assert s.pop() == 'd'

def set_pop_int():
    s: set[i32] = {1, 2, 3}

    assert s.pop() in {1, 2, 3}
    assert len(s) == 2
    assert s.pop() in {1, 2, 3}
    assert s.pop() in {1, 2, 3}
    assert len(s) == 0
    
    s.add(4)
    assert s.pop() == 4

set_pop_str()
set_pop_int()
