/**ARGS: --constant eval -DFOO=1 -DBAR */
/**SYSCODE: = 1 | 32 */
#if (1 + BAR) && defined(FOO)
KEEP ME
#endif
