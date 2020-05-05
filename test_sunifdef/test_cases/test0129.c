/**ARGS: --constant eval,del -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 16 */
/* Left(false,elim) && Right(false,elim) := Elim */
#if 0 && defined(FOO2)
DELETE ME
#else
KEEP ME
#endif
