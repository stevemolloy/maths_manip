#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define VERBOSE false

#define TODO() assert(0 && "Not yet implemented\n")

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

// A function has a name, arguments, and a body.
// The arguments are one or more expressions.
// The body is one or more expressions
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
    Sym *sym;
    NamedExpr *named_expr;
    Func *func;
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

SymMap new_sym_map(size_t ini_cap) {
  SymMap result = {0};
  result.cap = ini_cap;
  result.map = calloc(ini_cap, sizeof(SingleSymMap));
  return result;
}

int add_sym_to_map(Sym sym, Expr expr, SymMap *map) {
  if (map->len >= map->cap) {
    map->map = realloc(map->map, 2*map->cap * sizeof(SingleSymMap));
    if (map->map == NULL) {
      fprintf(stderr, "Memory problem\n");
      return -1;
    }
    map->cap *= 2;
  }
  map->map[map->len++] = (SingleSymMap){.sym = sym, .expr = expr};
  return 1;
}

int search_sym_map(SymMap map, Sym needle, Expr *return_expr) {
  for (size_t i=0; i<map.len; i++) {
    if (map.map[i].sym == needle) {
      *return_expr = map.map[i].expr;
      return 1;
    }
  }
  return -1;
}

Expr wrap_sym_in_expr(Sym *sym) {
  return (Expr){
    .type = SYM,
    .as.sym = sym,
  };
}

Expr wrap_func_in_expr(Func *func) {
  return (Expr) {
    .type = FUNC,
    .as.func = func
  };
}

Expr wrap_namedexpr_in_expr(NamedExpr *ne) {
  return (Expr) {
    .type = NAMED_EXPR,
    .as.named_expr = ne
  };
}

#define wrap_in_expr(X) _Generic((X),      \
              Sym*: wrap_sym_in_expr,      \
             Func*: wrap_func_in_expr,     \
        NamedExpr*: wrap_namedexpr_in_expr \
                                    )(X)

#define print_expr(a) _print_expr((a), 0)

void _print_expr(Expr expr, size_t level) {
  switch (expr.type) {
    case SYM:
      printf("%s", *expr.as.sym);
      break;
    case FUNC:
      printf("%s(", expr.as.func->name);
      _print_expr(*expr.as.func->head, level + 1);
      printf(") => ");
      _print_expr(*expr.as.func->body, level+1);
      break;
    case NAMED_EXPR:
      printf("%s(", expr.as.named_expr->name);
      for (size_t i=0; i<expr.as.named_expr->num_args; i++) {
        if (i>0) printf(", ");
        _print_expr(expr.as.named_expr->args[i], level + 1);
      }
      printf(")");
      break;
  }
  if (level==0) printf("\n");
}

bool match_exprs(Expr test_expr, Expr main_expr, SymMap *sym_map) {
#if VERBOSE
  printf("Checking if the following match:\n");
  printf("\t");
  print_expr(test_expr);
  printf("\t");
  print_expr(main_expr);
#endif

  if (main_expr.type == SYM) {
#if VERBOSE
    printf("\t\tMatching the test_expr against a SYM\n");
#endif
    add_sym_to_map(*main_expr.as.sym, test_expr, sym_map);
    return true;
  } else if (main_expr.type == FUNC) {
#if VERBOSE
    printf("\t\tMatching the test_expr against a FUNC\n");
#endif
    return match_exprs(test_expr, *main_expr.as.func->head, sym_map);
  } else if (main_expr.type == NAMED_EXPR) {
#if VERBOSE
    printf("\t\tMatching the test_expr against a NAMED_EXPR\n");
#endif
    if (test_expr.type != NAMED_EXPR) return false;
    if (test_expr.as.named_expr->name != main_expr.as.named_expr->name) return false;
    if (test_expr.as.named_expr->num_args != main_expr.as.named_expr->num_args) return false;
    for (size_t i=0; i<test_expr.as.named_expr->num_args; i++) {
      if (!match_exprs(test_expr.as.named_expr->args[i], main_expr.as.named_expr->args[i], sym_map))
        return false;
    }
  }
  return true;
}

int get_sym_equivalent(Sym sym, Expr f_head, Expr base_expr, Expr *equiv) {
  // e.g. sym=a, f_head=pair(a, b), base_expr=pair(x, y).
  // Should set equiv to x and return 1
  if (f_head.type == SYM) {
    if (sym == *f_head.as.sym) {
      *equiv = base_expr;
      return 1;
    }
  } else if (f_head.type == FUNC) {
    assert(0 && "Isn't this illegal?");
  } else if (f_head.type == NAMED_EXPR) {
    for (size_t i=0; i<f_head.as.named_expr->num_args; i++) {
      Expr head_arg = f_head.as.named_expr->args[i];
      Expr base_arg = base_expr.as.named_expr->args[i];
      if (get_sym_equivalent(sym, head_arg, base_arg, equiv) == 1) return 1;
    }
  } else {
    assert(0 && "Unreachable");
  }

  return 0;
}

// Expr execute_functor(Expr target, Func functor, SymMap sym_dict) { }

// Expr execute_functor(Expr f_head, Expr f_body, Expr target) {
//   if (f_head.type == SYM) {
//     Expr result = {0};
//     get_sym_equivalent(*f_head.as.sym, f_body, target, &result);
//     return result;
//   } else if (f_head.type == FUNC) {
//     assert(0 && "Isn't this illegal?");
//   } else if (f_head.type == NAMED_EXPR) {
//     for (size_t i=0; i<target.as.named_expr->num_args; i++) {
//       target.as.named_expr->args[i] = execute_functor(
//           f_head.as.named_expr->args[i],
//           f_body.as.named_expr->args[i],
//           target.as.named_expr->args[i]
//       );
//     }
//     return target;
//   }
//   assert(0 && "Unreachable");
// }

int main(void) {
  Sym a = "a", b = "b";

  NamedExpr pair_expr = {
    .name = "pair",
    .args = calloc(2, sizeof(Expr)),
    .num_args = 2,
  };
  pair_expr.args[0] = (Expr){.type=SYM, .as.sym=&a};
  pair_expr.args[1] = (Expr){.type=SYM, .as.sym=&b};

  NamedExpr swapped_pair_expr = {
    .name = "pair",
    .args = calloc(2, sizeof(Expr)),
    .num_args = 2,
  };
  swapped_pair_expr.args[0] = (Expr){.type=SYM, .as.sym=&b};
  swapped_pair_expr.args[1] = (Expr){.type=SYM, .as.sym=&a};

  Func swap = {
    .name = "swap",
    .head = calloc(1, sizeof(Expr)),
    .body = calloc(1, sizeof(Expr)),
  };
  swap.head[0] = (Expr){
    .type = NAMED_EXPR,
    .as.named_expr = &pair_expr,
  };
  swap.body[0] = (Expr){
    .type = NAMED_EXPR,
    .as.named_expr = &swapped_pair_expr,
  };

  Sym x = "x", y = "y";
  NamedExpr input = {
    .name = "pair",
    .args = calloc(2, sizeof(Expr)),
    .num_args = 2,
  };
  input.args[0] = (Expr){.type=SYM, .as.sym=&x};
  input.args[1] = (Expr){.type=SYM, .as.sym=&y};

  Expr input_expr = wrap_in_expr(&input);
  printf("Input:   ");
  print_expr(input_expr);

  printf("Functor: ");
  print_expr(wrap_in_expr(&swap));

  SymMap sym_map = new_sym_map(16);
  if (match_exprs(input_expr, wrap_in_expr(&swap), &sym_map)) {
    printf("They matched!\n");
  } else {
    printf("No match\n");
  }

  return 0;
}
