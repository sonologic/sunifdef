/**ARGS: --constant eval,del -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 16 */
/* Left(true,elim) && Right(true,elim) := Elim */
#if 1 && defined(FOO1)
KEEP ME
#else
DELETE ME
#endif
