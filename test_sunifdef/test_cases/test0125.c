/**ARGS: --constant eval,del -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 16 */
/* Left(true,elim) && Right(true,elim) := Elim */
#if 0 && defined(FOO1)
DELETE ME
#else
KEEP ME
#endif
