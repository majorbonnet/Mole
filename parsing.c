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

typedef enum { LVAL_INT, LVAL_FLOAT, LVAL_ERR } lval_type_t;
typedef enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM } lval_error_t;

typedef union lval_value
{
    long i;
    double d;
    lval_error_t err;   
} lval_value_t;

typedef struct 
{
    lval_type_t type;
    lval_value_t value;
} lval;

lval lval_int(long);
lval lval_float(double);
lval lval_err(lval_error_t);

void lval_print(lval);
void lval_println(lval);

lval eval_op(lval, char*, lval);
lval eval_float_op(double, char*, double);
lval eval_int_op(long, char*, long);
lval eval(mpc_ast_t*);

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
            lval result = eval(r.output);
            
            lval_println(result);
            
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

void lval_print(lval val)
{
    switch (val.type)
    {
        case LVAL_FLOAT:
            printf("%f", val.value.d);
            break;
        case LVAL_INT:
            printf("%d", val.value.i);
            break;
        case LVAL_ERR:
            if (val.value.err == LERR_DIV_ZERO) {
                printf("Error: Division By Zero!");
            }
            
            if (val.value.err == LERR_BAD_OP)   {
                printf("Error: Invalid Operator!");
            }
            
            if (val.value.err == LERR_BAD_NUM)  {
                printf("Error: Invalid Number!");
            }
            break;
    }   
}

void lval_println(lval val)
{
    lval_print(val);
    putchar('\n');
}

lval eval_op(lval x, char* op, lval y) {
    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }
    
    if (x.type == LVAL_INT && y.type == LVAL_INT) 
    {
        return eval_int_op(x.value.i, op, y.value.i);
    }
    
    if (x.type == LVAL_FLOAT && y.type == LVAL_INT)
    {
        return eval_float_op(x.value.d, op, (double)y.value.i);
    } 
    
    if (x.type == LVAL_INT && y.type == LVAL_FLOAT)
    {
        return eval_float_op((double)x.value.i, op, y.value.d);
    } 
    
    if (x.type == LVAL_FLOAT && y.type == LVAL_FLOAT)
    {
        return eval_float_op(x.value.d, op, y.value.d);
    } 
 
    return lval_err(LERR_BAD_OP);
}

lval eval_int_op(long x, char* op, long y)
{   
    if (strcmp(op, "+") == 0) 
    { 
        return lval_int(x + y);
    }
    
    if (strcmp(op, "-") == 0) 
    { 
        return lval_int(x - y);
    }
    
    if (strcmp(op, "*") == 0) 
    { 
        return lval_int(x * y);
    }
    
    if (strcmp(op, "/") == 0)
    { 
        return (y == 0 ? lval_err(LERR_DIV_ZERO) : lval_int(x / y));
    }
    
    if (strcmp(op, "%") == 0) 
    { 
        return lval_int(x % y);
    }
    
    return lval_err(LERR_BAD_OP);
}

lval eval_float_op(double x, char* op, double y)
{   
    if (strcmp(op, "+") == 0) 
    { 
        return lval_float(x + y);
    }
    
    if (strcmp(op, "-") == 0) 
    { 
        return lval_float(x - y);
    }
    
    if (strcmp(op, "*") == 0) 
    { 
        return lval_float(x * y);
    }
    
    if (strcmp(op, "/") == 0)
    { 
        return (y == 0 ? lval_err(LERR_DIV_ZERO) : lval_int(x / y));
    }
 
    return lval_err(LERR_BAD_OP);
}

lval lval_int(long x)
{
    lval val;
    val.type = LVAL_INT;
    val.value.i = x;
    
    return val;
}

lval lval_float(double x)
{
    lval val;
    val.type = LVAL_FLOAT;
    val.value.d = x;
    
    return val;
}

lval lval_err(lval_error_t err)
{
    lval val;
    val.type = LVAL_ERR;
    val.value.err = err;
    
    return val;
}

lval eval(mpc_ast_t* t) {
    if (strstr(t->tag, "integer")) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_int(x) : lval_err(LERR_BAD_NUM);
    }
    
    if (strstr(t->tag, "flt")) {
        return lval_float(atof(t-> contents));
    }
    
    char* op = t->children[1]->contents;
    
    lval x = eval(t->children[2]);
    
    int i = 3;
    
    while(strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }
    
    return x;
}

