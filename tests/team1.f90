PROGRAM team1
implicit none

INTEGER, PARAMETER          :: n = 4
TYPE (TEAM_TYPE)            :: column
REAL,CODIMENSION[n, *]      :: co_array
INTEGER,DIMENSION(2)        :: my_cosubscripts
my_cosubscripts (:)   = THIS_IMAGE(co_array)
! ​FORM TEAM (my_cosubscripts(2), column, NEW_INDEX = my_cosubscripts(1))
FORM TEAM (my_cosubscripts(2), column)
SYNC TEAM (column)
CHANGE TEAM (column, ca[*] => co_array)
! Segment 1
END TEAM

END PROGRAM
