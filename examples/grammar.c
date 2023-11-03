#include <stdio.h>
#include <stdlib.h>

typedef enum {
  SYM,
  FUNC,
  EXPR,
  NAMED_EXPR
} ExprType;

// A symbol is a c-string.
typedef char* Sym;

// Forward-declare Expr so I can use it in the function struct
struct Expr;

// A function has a name, arguments, and a body.
// The arguments are one or more expressions.
// The body is one or more expressions
typedef struct {
  char* name;
  struct Expr *args;
  struct Expr *body;
} Func;

// A "named expression" has a name and args, where args is one or more expressions
// This is supposed to allow representation of, e.g., "pair(a, b)"
typedef struct {
  char* name;
  struct Expr *args;
} NamedExpr;

// An expression is a symbol, an expression, a named expression, or a function.
typedef struct Expr {
  ExprType type;
  union {
    Sym *sym;
    struct Expr *expr;
    NamedExpr *named_expr;
    Func *func;
  } as;
} Expr;

int main(void) {
  Sym a = "a", b = "b";

  NamedExpr pair_expr = {
    .name = "pair",
    .args = calloc(2, sizeof(Expr)),
  };
  pair_expr.args[0] = (Expr){.type=SYM, .as.sym=&a};
  pair_expr.args[1] = (Expr){.type=SYM, .as.sym=&b};

  NamedExpr swapped_pair_expr = {
    .name = "pair",
    .args = calloc(2, sizeof(Expr)),
  };
  swapped_pair_expr.args[0] = (Expr){.type=SYM, .as.sym=&b};
  swapped_pair_expr.args[1] = (Expr){.type=SYM, .as.sym=&a};

  Func swap = {
    .name = "swap",
    .args = calloc(1, sizeof(Expr)),
    .body = calloc(1, sizeof(Expr)),
  };
  swap.args[0] = (Expr){
    .type = NAMED_EXPR,
    .as.named_expr = &pair_expr,
  };
  swap.body[0] = (Expr){
    .type = NAMED_EXPR,
    .as.named_expr = &swapped_pair_expr,
  };

  return 0;
}
