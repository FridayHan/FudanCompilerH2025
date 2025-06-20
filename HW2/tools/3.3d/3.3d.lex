%{
#include "parser.tab.h"
#include <string.h>
%}

%%
\(      { return OPEN_PAREN; }
\)      { return CLOSE_PAREN; }
\[      { return OPEN_BRACKET; }
\]      { return CLOSE_BRACKET; }
\n      { return END; }
[ \t]+  ;
.       { return ERROR; }
%%
int yywrap() { return 1; }
