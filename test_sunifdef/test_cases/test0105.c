/**ARGS: --constant eval,del -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 32 */
/* Left(true,elim) && Right(unk) := Right */
#if 1 && defined(UNKNOWN)
KEEP ME
#else
KEEP ME
#endif
