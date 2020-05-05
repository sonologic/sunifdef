/**ARGS: --constant eval -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 32 */
/* Left(true,keep) || Right(true,elim) := Left */
#if 1 || defined(FOO1)
KEEP ME
#else
DELETE ME
#endif
