/**ARGS: -DFOO=BAR -DBAR=FOO */
/**SYSCODE: = 0 */
#if FOO || BAR
KEEP ME
#else
KEEP ME
#endif
