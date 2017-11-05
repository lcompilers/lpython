results = [
    ('WhileLoop', (1, 1), ('Compare', (1, 1), ('Name', (1, 1), 'x'), ('Eq',), ('Name', (1, 1), 'y')), [('Assignment', (1, 1), 'i', ('BinOp', (1, 1), ('Name', (1, 1), 'i'), ('Add',), ('Num', (1, 1), '1')))]),
    ('Subroutine', (1, 1), 'a', [], [('If', (1, 1), ('Name', (1, 1), 'a'), [('Assignment', (1, 1), 'x', ('Num', (1, 1), '1'))], [('Assignment', (1, 1), 'x', ('Num', (1, 1), '2'))])]),
    ('Subroutine', (1, 1), 'a', [], [('If', (1, 1), ('Name', (1, 1), 'a'), [('Assignment', (1, 1), 'x', ('Num', (1, 1), '1'))], [('If', (1, 1), ('Name', (1, 1), 'b'), [('Assignment', (1, 1), 'x', ('Num', (1, 1), '2'))], [])])]),
    ('Subroutine', (1, 1), 'a', [], [('If', (1, 1), ('Name', (1, 1), 'a'), [('Assignment', (1, 1), 'x', ('Num', (1, 1), '1'))], [('If', (1, 1), ('Name', (1, 1), 'b'), [('Assignment', (1, 1), 'x', ('Num', (1, 1), '2'))], [('If', (1, 1), ('Name', (1, 1), 'c'), [('Assignment', (1, 1), 'x', ('Num', (1, 1), '2'))], [])])])]),
    ('Subroutine', (1, 1), 'a', [], [('If', (1, 1), ('Name', (1, 1), 'a'), [('Assignment', (1, 1), 'x', ('Num', (1, 1), '1'))], [])]),
    ('Subroutine', (1, 1), 'a', [], [('If', (1, 1), ('Name', (1, 1), 'a'), [('Assignment', (1, 1), 'x', ('Num', (1, 1), '1')), ('Assignment', (1, 1), 'y', ('Num', (1, 1), '2'))], [])]),
    ('Subroutine', (1, 1), 'a', [], [('If', (1, 1), ('Name', (1, 1), 'a'), [('Assignment', (1, 1), 'x', ('Num', (1, 1), '1'))], [])]),
    ('Subroutine', (1, 1), 'a', [], [('If', (1, 1), ('Name', (1, 1), 'a'), [('Assignment', (1, 1), 'x', ('Num', (1, 1), '1'))], [])]),
    ('Subroutine', (1, 1), 'a', [], [('If', (1, 1), ('Name', (1, 1), 'a'), [('Assignment', (1, 1), 'x', ('Num', (1, 1), '1'))], [])]),
    ('Subroutine', (1, 1), 'a', [], [('DoLoop', (1, 1), None, [('Assignment', (1, 1), 'x', ('Num', (1, 1), '1'))])]),
    ('Subroutine', (1, 1), 'a', [], [('DoLoop', (1, 1), ('do_loop_head', 'i', ('Num', (1, 1), '1'), ('Num', (1, 1), '5'), None), [('Assignment', (1, 1), 'x', ('BinOp', (1, 1), ('Name', (1, 1), 'x'), ('Add',), ('Name', (1, 1), 'i')))])]),
    ('Subroutine', (1, 1), 'a', [], [('DoLoop', (1, 1), ('do_loop_head', 'i', ('Num', (1, 1), '1'), ('Num', (1, 1), '5'), ('UnaryOp', (1, 1), ('USub',), ('Num', (1, 1), '1'))), [('Assignment', (1, 1), 'x', ('Name', (1, 1), 'i'))])]),
]
