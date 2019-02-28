results = [
    [('Assignment', (1, 1), ('Name', (1, 1), 'a'), ('Num', (1, 1), '5')), ('Assignment', (1, 1), ('Name', (1, 1), 'b'), ('Num', (1, 1), '6'))],
    [('Assignment', (1, 1), ('Name', (1, 1), 'a'), ('Num', (1, 1), '5')), ('Assignment', (1, 1), ('Name', (1, 1), 'b'), ('Num', (1, 1), '6'))],
    [('Assignment', (1, 1), ('Name', (1, 1), 'a'), ('Num', (1, 1), '5')), ('Print', (1, 1), None, [('Name', (1, 1), 'a')])],
    [('Assignment', (1, 1), ('Name', (1, 1), 'a'), ('Num', (1, 1), '5')), ('Subroutine', (1, 1), 'p', [], [], [], [('Print', (1, 1), None, [('Name', (1, 1), 'a')])], []), ('SubroutineCall', (1, 1), 'p', [])],
    [('Module', 'a', [], [('Declaration', (1, 1), [('decl', 'i', 'integer', [], [])])], []), ('Use', (1, 1), ('use_symbol', 'a', None), [('use_symbol', 'i', None)]), ('Assignment', (1, 1), ('Name', (1, 1), 'i'), ('Num', (1, 1), '5'))],
    [('Use', (1, 1), ('use_symbol', 'a', None), [('use_symbol', 'i', None)]), ('Assignment', (1, 1), ('Name', (1, 1), 'i'), ('Num', (1, 1), '5'))],
    [('Use', (1, 1), ('use_symbol', 'a', None), [('use_symbol', 'i', None)]), ('Assignment', (1, 1), ('Name', (1, 1), 'i'), ('Num', (1, 1), '5'))],
    ('SubroutineCall', (1, 1), 'plot', [('Name', (1, 1), 'x'), ('Name', (1, 1), 'y'), ('Str', (1, 1), 'o-')]),
    [('Assignment', (1, 1), ('Name', (1, 1), 'x'), ('ArrayInitializer', (1, 1), [('Num', (1, 1), '1'), ('Num', (1, 1), '2'), ('Num', (1, 1), '3')])), ('Assignment', (1, 1), ('Name', (1, 1), 'y'), ('ArrayInitializer', (1, 1), [('Num', (1, 1), '1'), ('Num', (1, 1), '2'), ('Num', (1, 1), '1')])), ('SubroutineCall', (1, 1), 'plot', [('Name', (1, 1), 'x'), ('Name', (1, 1), 'y'), ('Str', (1, 1), 'o-')])],
]
