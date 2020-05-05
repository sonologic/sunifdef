/**ARGS: --constant eval -DFOO1 -UFOO2 */
/**SYSCODE: = 0 */
/* Left(unk) || Right(false,keep) := Keep */
#if defined(UNKNOWN) || 0
KEEP ME
#else
KEEP ME
#endif
