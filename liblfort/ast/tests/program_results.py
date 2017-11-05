results = [
    ('Program', 'test', [('Declaration', (1, 1), [('decl', 'x', 'integer')])], [('Assignment', (1, 1), 'x', ('Num', (1, 1), '1')), ('SubroutineCall', (1, 1), 'a', [('Name', (1, 1), 'x')])], []),
    ('Program', 'test', [('Declaration', (1, 1), [('decl', 'x', 'integer')])], [('Assignment', (1, 1), 'x', ('Num', (1, 1), '1')), ('SubroutineCall', (1, 1), 'a', [('Name', (1, 1), 'x')])], [('Subroutine', (1, 1), 'a', [('arg', 'b')], [('Declaration', (1, 1), [('decl', 'b', 'integer')])], [], [])]),
    ('Program', 'test', [('Declaration', (1, 1), [('decl', 'x', 'integer')])], [('Assignment', (1, 1), 'x', ('Num', (1, 1), '1')), ('SubroutineCall', (1, 1), 'a', [('Name', (1, 1), 'x')])], [('Subroutine', (1, 1), 'a', [('arg', 'b')], [('Declaration', (1, 1), [('decl', 'b', 'integer')])], [], []), ('Subroutine', (1, 1), 'f', [('arg', 'b')], [('Declaration', (1, 1), [('decl', 'b', 'integer')])], [], [])]),
]
