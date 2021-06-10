program use_statement
    ! AST
    ! Syntax check only
    use, non_intrinsic :: example
    USE, NON_INTRINSIC :: EXAMPLE
    use, intrinsic :: example, only: name=>rename, NAME=>RENAME
    use example, only: sample
    use example, only: sample=>rename
    use, non_intrinsic :: example, only: assignment(=)
    use :: example, only: operator (+)
    use example, only: operator (-)
    use example, only: operator (*)
    use example, only: operator ( / )
    use, intrinsic :: example, only: operator (**)
    use, non_intrinsic :: example, only: operator (==)
    use example, only: operator (/=)
    use example, only: operator (>)
    use example, only: operator (>=)
    use example, only: operator (<)
    use example, only: operator (<=)
    use, intrinsic :: example, only: operator (.not.)
    use example, only: operator (.and.)
    use, non_intrinsic :: example, only: operator (.or.)
    use example, only: operator (.eqv.)
    use example, only: operator (.neqv.)
    use, intrinsic :: example, only: operator (.in.)
    use example, only: operator (.dot.)
    use, non_intrinsic :: example, only: operator (.definedoperator.)
end
