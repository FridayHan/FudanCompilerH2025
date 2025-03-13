%{
#include <stdio.h>
int yylex();
void yyerror(const char *s) { printf("reject\n"); }
%}

%token OPEN_PAREN CLOSE_PAREN OPEN_BRACKET CLOSE_BRACKET END ERROR

%%

S : T
  |
  ;

T : C T
  | C
  ;

C : OPEN_PAREN T CLOSE_PAREN
  | OPEN_BRACKET E CLOSE_BRACKET
  | OPEN_PAREN CLOSE_PAREN
  ;

E : S OPEN_PAREN E
  | S
  ;

%%
int main() {
  yyparse();
  return 0;
}