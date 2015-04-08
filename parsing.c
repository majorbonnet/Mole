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

typedef enum { LVAL_INT, LVAL_FLOAT, LVAL_ERR, LVAL_SYM, LVAL_SEXPR } lval_type;

typedef union lval_value
{
    long i;
    double d;
    char* sym;
    char* err;
} lval_value;

typedef struct lval
{
    lval_type type;
    lval_value value;
    int count; // number of child lvals
    struct lval** cell;
} lval;

lval* lval_int(long);
lval* lval_float(double);
lval* lval_sym(char*);
lval* lval_sexpr(void);
lval* lval_err(char*);
void lval_del(lval*);

void lval_expr_print(lval*, char, char);
void lval_print(lval*);
void lval_println(lval*);

lval* lval_eval(lval*);
lval* lval_pop(lval*, int);
lval* lval_take(lval*, int);
lval* eval_sexpr(lval*);
void eval_float_op(lval*, char*, lval*);
void eval_int_op(lval*, char*, lval*);

lval* lval_add(lval*, lval*);
lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t*);

int main(int argc, char** argv) {
    mpc_parser_t* Flt = mpc_new("flt");
	mpc_parser_t* Integer = mpc_new("integer");
	mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");
	
	mpca_lang(MPCA_LANG_DEFAULT,
		"                                                                     \
        integer     : /-?[0-9]+/ ;                                            \
        flt 		: /-?[0-9]+([.][0-9]+)/ ;	                              \
		symbol  	: '+' | '-' | '*' | '/'  | '%' ;			              \
        sexpr       : '(' <expr>* ')';                                       \
        expr		: <flt> | <integer> | <symbol> | <sexpr>;                 \
        lispy       : /^/ <expr>* /$/ ;                                       \
		",
		 Flt, Integer, Symbol, Sexpr, Expr, Lispy);

	puts("Lispy Version 0.0.0.0.1");
	puts("Press Ctrl+c to exit\n");
	
	while (1) {
		char* input = readline("lispy> ");
		add_history(input);
		
		mpc_result_t r;
        
        if(mpc_parse("<stdin>", input, Lispy, &r)) {
            lval* result = lval_eval(lval_read(r.output)); 
            lval_println(result);
            lval_del(result);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        
		free(input);
	}
	
    mpc_cleanup(6, Flt, Integer, Symbol, Sexpr, Expr, Lispy);
    
	return 0;
}

void lval_expr_print(lval* val, char open, char close)
{
    putchar(open);
    for (int i=0; i < val->count; i++)
    {
        lval_print(val->cell[i]);
        
        if (i != val->count-1)
        {
            putchar(' ');
        }
    }
    
    putchar(close);
}

void lval_print(lval* val)
{
    switch(val->type)
    {
        case LVAL_INT: printf("%d", val->value.i); break;
        case LVAL_FLOAT: printf("%f", val->value.d); break;
        case LVAL_SYM: printf("%s", val->value.sym); break;
        case LVAL_ERR: printf("Error: %s", val->value.err); break;
        case LVAL_SEXPR: lval_expr_print(val, '(', ')'); break;
    }
}

void lval_println(lval* val)
{
    lval_print(val);
    putchar('\n');
}

lval* lval_eval(lval* val) 
{
  if (val->type == LVAL_SEXPR) { return eval_sexpr(val); }

  return val;
}

lval* lval_pop(lval* val, int i) 
{
  lval* x = val->cell[i];

  memmove(&val->cell[i], &val->cell[i+1],
    sizeof(lval*) * (val->count-i-1));

  val->count--;

  val->cell = realloc(val->cell, sizeof(lval*) * val->count);
  return x;
}

lval* lval_take(lval* val, int i) 
{
  lval* x = lval_pop(val, i);
  lval_del(val);
  return x;
}

lval* builtin_op(lval* a, char* op) 
{
  lval_type op_type = LVAL_INT;
  for (int i = 0; i < a->count; i++) 
  {
    if (a->cell[i]->type != LVAL_INT && a->cell[i]->type != LVAL_FLOAT) 
    {
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
    
    if (a->cell[i]->type == LVAL_FLOAT)
    {
        op_type = LVAL_FLOAT;
    }
  }
  
  lval* x = lval_pop(a, 0);

  if ((strcmp(op, "-") == 0) && a->count == 0) 
  {
      if (x->type == LVAL_INT)
      {
          x->value.i = -x->value.i;
      }
      
      if (x->type == LVAL_FLOAT)
      {
          x->value.d = -x->value.d;
      }
  }

  while (a->count > 0) 
  {
    lval* y = lval_pop(a, 0);
    
    switch (op_type)
    {
        case LVAL_INT:
            eval_int_op(x, op, y);
            break;
        case LVAL_FLOAT:
            eval_float_op(x, op, y);
            break;
        case LVAL_ERR: break;
        case LVAL_SYM: break;
        case LVAL_SEXPR: break;
    }

    lval_del(y);
  }

  lval_del(a); 
  
  return x;
}

lval* eval_sexpr(lval* val) 
{
  for (int i = 0; i < val->count; i++) 
  {
    val->cell[i] = lval_eval(val->cell[i]);
  }

  for (int i = 0; i < val->count; i++) 
  {
    if (val->cell[i]->type == LVAL_ERR) { return lval_take(val, i); }
  }

  if (val->count == 0) { return val; }

  if (val->count == 1) { return lval_take(val, 0); }

  lval* f = lval_pop(val, 0);
  if (f->type != LVAL_SYM) 
  {
    lval_del(f); lval_del(val);
    return lval_err("S-expression Does not start with symbol!");
  }

  lval* result = builtin_op(val, f->value.sym);
  lval_del(f);
  return result;
}

void eval_int_op(lval* x, char* op, lval* y)
{   
    if (strcmp(op, "+") == 0) 
    { 
        x->value.i += y->value.i;
        
        return;
    }
    
    if (strcmp(op, "-") == 0) 
    { 
        x->value.i -= y->value.i;
        
        return;
    }
    
    if (strcmp(op, "*") == 0) 
    { 
        x->value.i *= y->value.i;
        
        return;
    }
    
    if (strcmp(op, "/") == 0)
    { 
        if (y->value.i == 0) 
        { 
            lval_del(x);
            x = lval_err("Division by zero");
            
            
        }
        else
        {
            x->value.i /= y->value.i;
        }
        
        return;
    }
    
    if (strcmp(op, "%") == 0) 
    { 
        x->value.i = x->value.i % y->value.i;
        
        return;
    }
    
    lval_del(x);
    x = lval_err("Bad operation");
}

void eval_float_op(lval* x, char* op, lval* y)
{   
    double temp;
    if (x->type == LVAL_INT)
    {
        x->type = LVAL_FLOAT;
        
        temp = (double)x->value.i;
        x->value.d = temp;
    }

    if (y->type == LVAL_INT)
    {
        temp = (double)y->value.i;
        y->value.d = temp;
    }

    if (strcmp(op, "+") == 0) 
    { 
        x->value.d += y->value.d;
        
        return;
    }
    
    if (strcmp(op, "-") == 0) 
    { 
        x->value.d -= y->value.d;
        
        return;
    }
    
    if (strcmp(op, "*") == 0) 
    { 
        x->value.d *= y->value.d;
        
        return;
    }
    
    if (strcmp(op, "/") == 0)
    { 
        if (y->value.d == 0) 
        { 
            lval_del(x);
            x = lval_err("Division by zero");
            
            
        }
        else
        {
            x->value.d /= y->value.d;
        }
        
        return;
    }
        
    lval_del(x);
    x = lval_err("Bad operation");
}

lval* lval_int(long x)
{
    lval* val = malloc(sizeof(lval));
    val->type = LVAL_INT;
    val->value.i = x;
    
    return val;
}

lval* lval_float(double x)
{
    lval* val = malloc(sizeof(lval));
    val->type = LVAL_FLOAT;
    val->value.d = x;
    
    return val;
}

lval* lval_err(char* m)
{
    lval* val = malloc(sizeof(lval));
    val->type = LVAL_ERR;
    val->value.err = malloc(strlen(m) + 1);
    strcpy(val->value.err, m);
    
    return val;
}

lval* lval_sym(char* s)
{
    lval* val = malloc(sizeof(lval));
    val->type = LVAL_SYM;
    val->value.sym = malloc(strlen(s) + 1);
    strcpy(val->value.sym, s);
    
    return val;
}

lval* lval_sexpr(void)
{
    lval* val = malloc(sizeof(lval));
    val->type = LVAL_SEXPR;
    val->count = 0;
    val->cell = NULL;
    
    return val;
}

void lval_del(lval* val)
{
    switch (val->type)
    {
        case LVAL_INT: break;
        case LVAL_FLOAT: break;
        
        case LVAL_ERR: 
            free(val->value.err); 
            break;
        case LVAL_SYM: 
            free(val->value.sym); 
            break;
        
        case LVAL_SEXPR:
            for (int i = 0; i < val->count; i++)
            {
                lval_del(val->cell[i]);
            }
            
            free(val->cell);
            break;
    }
    
    free(val);
}

lval* lval_read_num(mpc_ast_t* t)
{
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_int(x) : lval_err("Invalid number"); 
}

lval* lval_add(lval* val, lval* x)
{
    val->count++;
    val->cell = realloc(val->cell, sizeof(lval*) * val->count);
    val->cell[val->count - 1] = x;
    
    return val;
}

lval* lval_read(mpc_ast_t* t) {
    if (strstr(t->tag, "integer")) {
        return lval_read_num(t);
    }
    
    if (strstr(t->tag, "flt")) {
        return lval_float(atof(t-> contents));
    }
    
    if (strstr(t->tag, "symbol")) {
        return lval_sym(t-> contents);
    }
    
    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }
    
    for (int i = 0; i < t->children_num; i++)
    {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }
    
    return x;
}