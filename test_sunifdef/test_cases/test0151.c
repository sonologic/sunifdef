/**ARGS: -c --constant eval,del -DFOO1=1 -UFOO2 */
/**SYSCODE: = 1 | 16 | 32 */
#if !(1 && FOO1)
DELETE ME
#else
KEEP ME
#endif
