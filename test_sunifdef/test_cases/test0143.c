/**ARGS: --constant eval -DFOO1 -UFOO2 */
/**SYSCODE: = 0 */
/* Left(true,keep) || Right(unk) := Keep */
#if 0 || defined(UNKNOWN)
KEEP ME
#else
KEEP ME
#endif
