%{
#include <stdio.h>
#include <string.h>
%}

%%

a((b|a*c)x)*    { printf("accept\n"); return 0; }
x*a             { printf("accpet\n"); return 0; }
.*              { printf("reject\n"); return 0; }

%%

int main() {
    char input[1024];
    while (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;

        yy_scan_string(input);
        yylex();
    }
    return 0;
}
