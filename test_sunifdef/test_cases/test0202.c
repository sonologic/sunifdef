/**ARGS: -DFOO -DBAR -UFIXME -kb */
/**SYSCODE: = 35 */
#if defined(FOO) \
    defined(BAR)
int main(void)
#else
int main(int argc, char* argv)
#endif
{
  bar();

#ifdef FIXME
/* multi
line
comment */
#endif
}
