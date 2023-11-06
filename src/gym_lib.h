#ifndef _GYM_LIB_H
#define _GYM_LIB_H

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Types of expressions
typedef enum {
  SYM,        // Simple symbols
  FUNC,       // Functions
  NAMED_EXPR  // Named expressions
} ExprType;

// A symbol is a c-string.
typedef char* Sym;

// Forward-declare Expr so I can use it in the function struct
struct Expr;

// A function has a name, arguments, and a body. The arguments/body are one or more expressions.
typedef struct {
  char* name;
  struct Expr *head;
  struct Expr *body;
} Func;

// A "named expression" has a name and args, where args is one or more expressions
// This is supposed to allow representation of, e.g., "pair(a, b)"
typedef struct {
  char* name;
  struct Expr *args;
  size_t num_args;
} NamedExpr;

// An expression is a symbol, an expression, a named expression, or a function.
typedef struct Expr {
  ExprType type;
  union {
    Sym sym;
    NamedExpr named_expr;
    Func func;
  } as;
} Expr;

typedef struct {
  Sym sym;
  Expr expr;
} SingleSymMap;

typedef struct {
  size_t len;
  size_t cap;
  SingleSymMap *map;
} SymMap;

Sym make_sym(char *name);

SymMap new_sym_map(size_t ini_cap);

int add_sym_to_map(Sym sym, Expr expr, SymMap *map);

int search_sym_map(SymMap map, Sym needle, Expr *return_expr);

void free_sym_map(SymMap *map);

Expr wrap_sym_in_expr(Sym sym);

Expr wrap_func_in_expr(Func func);

Expr wrap_namedexpr_in_expr(NamedExpr ne);

#define wrap_in_expr(X) _Generic((X),      \
              Sym: wrap_sym_in_expr,      \
             Func: wrap_func_in_expr,     \
        NamedExpr: wrap_namedexpr_in_expr \
                                    )(X)

#define print_expr(a) _print_expr((a), 0)

void _print_expr(Expr expr, size_t level);

bool match_exprs(Expr test_expr, Expr main_expr, SymMap *sym_map);

int get_sym_equivalent(Sym sym, Expr f_head, Expr base_expr, Expr *equiv);

Expr execute_functor(Expr target, Expr f_head, Expr f_body, SymMap sym_map);

#endif // !_GYM_LIB_H

