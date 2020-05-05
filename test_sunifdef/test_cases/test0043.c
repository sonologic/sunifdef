/**ARGS: -DFOO1 -UFOO2 */
/**SYSCODE: = 1 | 16 */
#ifdef FOO1
KEEP ME
#define OTHER
#endif
#ifdef OTHER
KEEP ME
#else
KEEP ME
#endif

