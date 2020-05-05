/**ARGS: -DFOO3 -kb */
/**SYSCODE: = 33 */
#if defined(FOO1)
foo1();
#elif defined(FOO2) || \
      defined(FOO3) || \
      defined(FOO4)
foo234();
#endif
