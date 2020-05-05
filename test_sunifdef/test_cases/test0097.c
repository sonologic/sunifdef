/**ARGS: --constant eval -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 32 */
/* Left(true,elim) && Right(true,keep) := Right */
#if defined(FOO1) && 1
KEEP ME
#else
KEEP ME
#endif
