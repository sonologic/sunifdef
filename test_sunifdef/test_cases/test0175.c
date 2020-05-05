/**ARGS: --pod -DFOO -DBAR */
/**SYSCODE: = 1 | 16 */
#ifdef FOO
/* Not to be parsed as comment
#if defined(BAR)
KEEP ME
#else
DELETE ME
#endif
end */
" Not to be parsed as quotation
#else
DELETE ME
#endif
end "
