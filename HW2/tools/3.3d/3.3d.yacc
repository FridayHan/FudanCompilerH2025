%{
#include <stdio.h>
int yylex();
void yyerror(const char *s) { printf("reject\n"); }
%}

%token OPEN_PAREN CLOSE_PAREN OPEN_BRACKET CLOSE_BRACKET END ERROR

%%

S : S S
  | OPEN_PAREN S CLOSE_PAREN
  | OPEN_BRACKET P CLOSE_BRACKET
  | /* empty */
  ;

P : OPEN_PAREN
  | OPEN_PAREN P CLOSE_PAREN
  | OPEN_BRACKET P CLOSE_BRACKET
  | P P
  | /* empty */
  ;

%%
int main() {
  if (yyparse() == 0) {
    printf("accept\n");
  }
  return 0;
}