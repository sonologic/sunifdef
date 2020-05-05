/**ARGS: --constant eval,del -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 32 */
#if !(1 && FOO1)
KEEP ME
#else
KEEP ME
#endif
