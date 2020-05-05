/**ARGS: --constant eval -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 32 */
/* Left(unk) || Right(true,keep) := Right */
#if defined(UNKNOWN) || 1
KEEP ME
#else
KEEP ME
#endif
