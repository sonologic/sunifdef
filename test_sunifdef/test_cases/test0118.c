/**ARGS: --constant eval,del -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 16 */
/* Left(true,elim) || Right(unk) := Elim */
#if 1 || defined(UNKNOWN)
KEEP ME
#else
DELETE ME
#endif
