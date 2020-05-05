/**ARGS: --constant eval -DFOO1 -UFOO2 */
/**SYSCODE: = 0 */
/* Left(true,keep) && Left(unk) := Keep */
#if 1 && defined(UNKNOWN)
KEEP ME
#else
KEEP ME
#endif
