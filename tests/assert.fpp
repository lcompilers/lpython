#ifdef NDEBUG
# define ASSERT(x) !! assert( x )
#else
# define ASSERT(x) if(.not.(x)) call f90_assert(__FILE__,__LINE__)
#endif
