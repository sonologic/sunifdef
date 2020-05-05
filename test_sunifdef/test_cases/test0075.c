/**ARGS: -DFOO1 -UFOO2 */
/**SYSCODE: = 0 */
// None of these literals should an unbalanced quote or unbalanced
// comment error
static const char * str1 = "'";
static const char * str2 = "\'";
static const char ch1 = '"';
static const char ch2 = '\"';
static const char *str3 = "//";
static const char *str4 = "/*";
static const char str5 = "*/";
static const char str6 ="\
text\
";
static const char *str7 = "\
/* This is not a comment */";
