#include <stdio.h>
#include <stdlib.h>
#include "lib\mpc.h"

#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt) {
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer) + 1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy) - 1] = '\0';
	return cpy;
}

/* Fake add history function */
void add_history(char* unused) {}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

double eval_op(double, char*, double);
double eval(mpc_ast_t*);

int main(int argc, char** argv) {
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");
	
	mpca_lang(MPCA_LANG_DEFAULT,
		"											                    \
		number		: /-?[0-9]+([.][0-9]+)?/ ;					            \
		operator	: '+' | '-' | '*' | '/' ;			\
		expr		: <number> | '(' <operator> <expr>+ ')' ;           \
        lispy       : /^/ <operator> <expr>+ /$/ ;                      \
		",
		 Number, Operator, Expr, Lispy);

	puts("Lispy Version 0.0.0.0.1");
	puts("Press Ctrl+c to exit\n");
	
	while (1) {
		char* input = readline("lispy> ");
		add_history(input);
		
		mpc_result_t r;
        
        if(mpc_parse("<stdin>", input, Lispy, &r)) {
            double result = eval(r.output);
            printf("%f\n", result);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        
		free(input);
	}
	
    mpc_cleanup(4, Number, Operator, Expr, Lispy);
    
	return 0;
}

double eval_op(double x, char* op, double y) {
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    
    return 0;
}

double eval(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) {
        return atof(t-> contents);
    }
    
    char* op = t->children[1]->contents;
    
    double x = eval(t->children[2]);
    
    int i = 3;
    
    while(strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }
    
    return x;
}

