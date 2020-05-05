/**ARGS: --constant eval -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 32 */
/* Left(true,keep) && Right(true,elim) := Left */
#if 0 && defined(FOO1)
KEEP ME
#else
KEEP ME
#endif
