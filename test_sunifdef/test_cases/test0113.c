/**ARGS: --constant eval -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 32 */
/* Left(false,elim) || Right(true,keep) := Right */
#if defined(FOO2) || 1
KEEP ME
#else
KEEP ME
#endif
