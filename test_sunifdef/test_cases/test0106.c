/**ARGS: --constant eval,del -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 32 */
/* Left(unk) && Left(true,elim) := Left */
#if defined(UNKNOWN) && 1
KEEP ME
#else
KEEP ME
#endif
