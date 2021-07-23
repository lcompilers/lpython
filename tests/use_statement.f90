program use_statement
    ! AST
    ! Syntax check only
    use, intrinsic     :: example, only: name=>rename, NAME=>RENAME
    use, intrinsic     :: example, only: operator (**)
    use, intrinsic     :: example, only: operator (.not.)
    use, intrinsic     :: example, only: operator (.in.)
    use, intrinsic     :: example, only: operator (.plus.) => operator (.add.)
    use, non_intrinsic :: example
    USE, NON_INTRINSIC :: EXAMPLE
    use, non_intrinsic :: example, only: assignment(=)
    use, non_intrinsic :: example, only: operator (==)
    use, non_intrinsic :: example, only: operator (.or.)
    use, non_intrinsic :: example, only: operator (.definedoperator.)
    use :: example, only: operator (+)
    use :: example, only: sample
    use :: example, only: sample=>rename
    use example, only: operator (*)
    use example, only: operator ( / )
    use example, only: operator (/)
    use example, only: operator (/=)
    use example, only: operator (>)
    use example, only: operator (<=)
    use example, only: operator (.and.)
    use example, only: operator (.eqv.)
    use example, only: operator (.dot.)
    use example, only: write(formatted)
    use example, only: read(unformatted)
    use example, only:
    use example, operator (.localDefop.) => operator (.useDefop.), a => b
end
