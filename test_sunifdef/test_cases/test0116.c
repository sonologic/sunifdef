/**ARGS: --constant eval -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 32 */
/* Left(true,keep) || Right(unk) := Left */
#if 1 || defined(UNKNOWN)
KEEP ME
#else
KEEP ME
#endif
