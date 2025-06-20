%{
#include <stdio.h>
%}

%%

ca[tr]s?   { printf("accept\n"); }
.|\n       { printf("reject\n"); }

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
