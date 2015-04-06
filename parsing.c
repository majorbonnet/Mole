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

typedef struct 
{
    char* type;
    int i;
    double d;
} Value;

Value eval_op(Value, char*, Value);
double eval_float_op(double, char*, double);
int eval_int_op(int, char*, int);
Value eval(mpc_ast_t*);

int main(int argc, char** argv) {
    mpc_parser_t* Flt = mpc_new("flt");
	mpc_parser_t* Integer = mpc_new("integer");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");
	
	mpca_lang(MPCA_LANG_DEFAULT,
		"                                               \
        integer     : /-?[0-9]+/ ;                                       \
        flt 		: /-?[0-9]+([.][0-9]+)/ ;	        \
		operator	: '+' | '-' | '*' | '/'  | '%' ;			            \
        expr		: <flt> | <integer> | '(' <operator> <expr>+ ')' ;           \
        lispy       : /^/ <operator> <expr>+ /$/ ;                            \
		",
		 Flt, Integer, Operator, Expr, Lispy);

	puts("Lispy Version 0.0.0.0.1");
	puts("Press Ctrl+c to exit\n");
	
	while (1) {
		char* input = readline("lispy> ");
		add_history(input);
		
		mpc_result_t r;
        
        if(mpc_parse("<stdin>", input, Lispy, &r)) {
            Value result = eval(r.output);
            
            if (strstr(result.type, "flt"))
            {
                printf("%f\n", result.d);
            }
            else
            {
                printf("%d\n", result.i);
            }
            
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        
		free(input);
	}
	
    mpc_cleanup(4, Flt, Integer, Operator, Expr, Lispy);
    
	return 0;
}

Value eval_op(Value x, char* op, Value y) {
    Value val;
    
    // there has to be a more efficient way to structure this
    // I need to read up on my C again.
    if (strstr(x.type, "integer") && strstr(y.type, "integer")) 
    {
        val.type = "integer\0";
        val.i = eval_int_op(x.i, op, y.i);
        
        return val;
    }
    
    if (strstr(x.type, "flt") && strstr(y.type, "integer"))
    {
        val.type = "flt\0";
        val.d = eval_float_op(x.d, op, (double)y.i);
        return val;
    } 
    
    if (strstr(x.type, "integer") && strstr(y.type, "flt"))
    {
        val.type = "flt\0";
        val.d = eval_float_op((double)x.i, op, y.d);
        return val;
    } 
    
    if (strstr(x.type, "flt") && strstr(y.type, "flt"))
    {
        val.type = "flt\0";
        val.d = eval_float_op(x.d, op, y.d);
        return val;
    } 

    // default return value
    // should really throw an error if we hit this point    
    val.type = "integer\0";
    val.i = 0;
    return val;
}

int eval_int_op(int x, char* op, int y)
{
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    if (strcmp(op, "%") == 0) { return x % y; }
      
    return 0;
}

double eval_float_op(double x, char* op, double y)
{
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
     
    return 0;
}

Value eval(mpc_ast_t* t) {
    Value val;
    if (strstr(t->tag, "integer")) {
        val.type = "integer\0";
        val.i = atoi(t-> contents);
        return val;
    }
    
    if (strstr(t->tag, "flt")) {
        val.type = "flt\0";
        val.d = atof(t-> contents);
        return val;
    }
    
    char* op = t->children[1]->contents;
    
    Value x = eval(t->children[2]);
    
    int i = 3;
    
    while(strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }
    
    return x;
}

