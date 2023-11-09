#include <string.h>

#include "gym_lib.h"

Sym make_sym(char *name) {
  size_t len = strlen(name) + 1;
  Sym result = calloc(len, sizeof(char));
  memcpy(result, name, len);
  return result;
}

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
    if (strcmp(map.map[i].sym, needle) == 0) {
      free_expr(return_expr);
      *return_expr = map.map[i].expr;
      return 1;
    }
  }
  return -1;
}

void free_sym_map(SymMap *map) {
  free(map->map);
}

void free_expr(Expr *expr) {
  switch (expr->type) {
    case FUNC:
      free_expr(expr->as.func.body);
      free_expr(expr->as.func.head);
      free(expr->as.func.body);
      free(expr->as.func.head);
      free(expr->as.func.name);
      break;
    case SYM:
      free(expr->as.sym);
      break;
    case NAMED_EXPR:
      for (size_t i=0; i<expr->as.named_expr.num_args; i++) {
        free_expr(&expr->as.named_expr.args[i]);
      }
      free(expr->as.named_expr.args);
      free(expr->as.named_expr.name);
      break;
  }
}

Expr wrap_sym_in_expr(Sym sym) {
  return (Expr){
    .type = SYM,
    .as.sym = sym,
  };
}

Expr wrap_func_in_expr(Func func) {
  return (Expr) {
    .type = FUNC,
    .as.func = func
  };
}

Expr wrap_namedexpr_in_expr(NamedExpr ne) {
  return (Expr) {
    .type = NAMED_EXPR,
    .as.named_expr = ne
  };
}

#define wrap_in_expr(X) _Generic((X),      \
              Sym: wrap_sym_in_expr,      \
             Func: wrap_func_in_expr,     \
        NamedExpr: wrap_namedexpr_in_expr \
                                    )(X)

#define print_expr(a) _print_expr((a), 0)

void _print_expr(Expr expr, size_t level) {
  switch (expr.type) {
    case SYM:
      printf("%s", expr.as.sym);
      break;
    case FUNC:
      printf("%s(", expr.as.func.name);
      _print_expr(*expr.as.func.head, level + 1);
      printf(") => ");
      _print_expr(*expr.as.func.body, level+1);
      break;
    case NAMED_EXPR:
      printf("%s(", expr.as.named_expr.name);
      for (size_t i=0; i<expr.as.named_expr.num_args; i++) {
        if (i>0) printf(", ");
        _print_expr(expr.as.named_expr.args[i], level + 1);
      }
      printf(")");
      break;
  }
  if (level==0) printf("\n");
}

bool compare_expressions(Expr a, Expr b) {
  if (a.type != b.type) return false;
  switch (a.type) {
    case SYM:
      return strcmp(a.as.sym, b.as.sym)==0;
    case NAMED_EXPR:
      if (strcmp(a.as.named_expr.name, b.as.named_expr.name)!=0) return false;
      if (a.as.named_expr.num_args != b.as.named_expr.num_args) return false;
      for (size_t i=0; i<a.as.named_expr.num_args; i++) {
        if (!compare_expressions(a.as.named_expr.args[i], b.as.named_expr.args[i])) return false;
      }
      return true;
    case FUNC:
      if (strcmp(a.as.func.name, b.as.func.name)!=0) return false;
      if (!compare_expressions(*a.as.func.head, *b.as.func.head)) return false;
      if (!compare_expressions(*a.as.func.body, *b.as.func.body)) return false;
      return true;
  }
}

bool match_exprs(Expr test_expr, Expr main_expr, SymMap *sym_map) {
  if (main_expr.type == SYM) {
    Expr check_expr = {0};
    if (search_sym_map(*sym_map, main_expr.as.sym, &check_expr) < 0) {
      add_sym_to_map(main_expr.as.sym, test_expr, sym_map);
    } else {
      return compare_expressions(check_expr, test_expr);
    }
    return true;
  } else if (main_expr.type == FUNC) {
    return match_exprs(test_expr, *main_expr.as.func.head, sym_map);
  } else if (main_expr.type == NAMED_EXPR) {
    if (test_expr.type != NAMED_EXPR) return false;
    if (strcmp(test_expr.as.named_expr.name, main_expr.as.named_expr.name) != 0) return false;
    if (test_expr.as.named_expr.num_args != main_expr.as.named_expr.num_args) return false;
    for (size_t i=0; i<test_expr.as.named_expr.num_args; i++) {
      if (!match_exprs(test_expr.as.named_expr.args[i], main_expr.as.named_expr.args[i], sym_map))
        return false;
    }
  }
  return true;
}

Expr execute_functor(Expr func_body, SymMap sym_map) {
  // Given that we have already confirmed the match between the target and the functor,
  // we do not actually need the target to be provided.  All we need is the functor body
  // and the sym_map to provide the translation.
  // The functor body is provided as an expression to allow for resursion.
  // In order to avoid mem allocs, I alter func_body and then return it.
  switch (func_body.type) {
    case SYM:
      search_sym_map(sym_map, func_body.as.sym, &func_body);
      break;
    case NAMED_EXPR:
      for (size_t i=0; i<func_body.as.named_expr.num_args; i++) {
        func_body.as.named_expr.args[i] = execute_functor(func_body.as.named_expr.args[i], sym_map);
      }
      break;
    case FUNC:
      TODO();
      break;
  }
  return func_body;
}

