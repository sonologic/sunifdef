/**ARGS: -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 32 */
/* Left(unk) || Right(false,elim) := Left */
#if 0 || defined(FOO2)
KEEP ME
#else
KEEP ME
#endif
