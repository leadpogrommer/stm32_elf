#include <stdio.h>


int prog_main(){
    printf("Welcome to my calculator\n");
    while(1){
        printf("Enter expression: ");
        int o1, o2;
        char op;
        if(scanf("%d %c %d", &o1, &op, &o2) != 3){
            printf("Syntax error, exiting\n");
            return 0;
        }
        int res;
        switch(op){
            case '+': res = o1 + o2; break;
            case '-': res = o1 - o2; break;
            case '*': res = o1 * o2; break;
            case '/': res = o1 / o2; break;
            default: printf("Unknown operation, exiting\n"); return 0;
        }
        printf("= %d\n", res);
    }
}