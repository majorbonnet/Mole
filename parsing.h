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