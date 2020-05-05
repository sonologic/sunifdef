/**ARGS: --constant eval -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 32 */
/* Left(true,keep) || Right(false,elim) := Left */
#if 1 || defined(FOO2)
KEEP ME
#else
KEEP ME
#endif
