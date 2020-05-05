/**ARGS: --constant eval,del -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 32 */
/* Left(false,elim) || Right(unk) := Right */
#if 0 || defined(UNKNOWN)
KEEP ME
#else
KEEP ME
#endif
