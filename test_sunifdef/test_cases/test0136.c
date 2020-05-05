/**ARGS: --constant eval,del -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 16 */
/* Left(false,elim) || right(true.elim) := Elim */
#if 0 || defined(FOO1)
KEEP ME
#else
DELETE ME
#endif
