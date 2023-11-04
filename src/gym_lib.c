#include "gym_lib.h"

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

void free_sym_map(SymMap *map) {
  free(map->map);
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
    add_sym_to_map(main_expr.as.sym, test_expr, sym_map);
    return true;
  } else if (main_expr.type == FUNC) {
#if VERBOSE
    printf("\t\tMatching the test_expr against a FUNC\n");
#endif
    return match_exprs(test_expr, *main_expr.as.func.head, sym_map);
  } else if (main_expr.type == NAMED_EXPR) {
#if VERBOSE
    printf("\t\tMatching the test_expr against a NAMED_EXPR\n");
#endif
    if (test_expr.type != NAMED_EXPR) return false;
    if (test_expr.as.named_expr.name != main_expr.as.named_expr.name) return false;
    if (test_expr.as.named_expr.num_args != main_expr.as.named_expr.num_args) return false;
    for (size_t i=0; i<test_expr.as.named_expr.num_args; i++) {
      if (!match_exprs(test_expr.as.named_expr.args[i], main_expr.as.named_expr.args[i], sym_map))
        return false;
    }
  }
  return true;
}

int get_sym_equivalent(Sym sym, Expr f_head, Expr base_expr, Expr *equiv) {
  // e.g. sym=a, f_head=pair(a, b), base_expr=pair(x, y).
  // Should set equiv to x and return 1
  if (f_head.type == SYM) {
    if (sym == f_head.as.sym) {
      *equiv = base_expr;
      return 1;
    }
  } else if (f_head.type == FUNC) {
    assert(0 && "Isn't this illegal?");
  } else if (f_head.type == NAMED_EXPR) {
    for (size_t i=0; i<f_head.as.named_expr.num_args; i++) {
      Expr head_arg = f_head.as.named_expr.args[i];
      Expr base_arg = base_expr.as.named_expr.args[i];
      if (get_sym_equivalent(sym, head_arg, base_arg, equiv) == 1) return 1;
    }
  } else {
    assert(0 && "Unreachable");
  }

  return 0;
}

Expr execute_functor(Expr target, Expr f_head, Expr f_body, SymMap sym_map) {
  if (f_head.type == SYM) {
    Expr return_value = {0};
    search_sym_map(sym_map, f_head.as.sym, &return_value);
    return return_value;
  } else if (f_head.type == FUNC) {
    assert(0 && "Isn't this illegal?");
  } else if (f_head.type == NAMED_EXPR) {
    for (size_t i=0; i<target.as.named_expr.num_args; i++) {
      target.as.named_expr.args[i] = execute_functor(
          f_head.as.named_expr.args[i],
          f_body.as.named_expr.args[i],
          target.as.named_expr.args[i],
          sym_map
      );
    }
    return target;
  }
  assert(0 && "Unreachable");
}

