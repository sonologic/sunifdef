/**ARGS: -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 32 */
/* Left(unk) && Right(true,elim) := Left */
#if 1 && defined(FOO1)
KEEP ME
#else
KEEP ME
#endif
