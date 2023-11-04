#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "gym_lib.h"

#define VERBOSE false

#define TODO() assert(0 && "Not yet implemented\n")

int main(void) {
  NamedExpr pair_expr = {
    .name = "pair",
    .args = calloc(2, sizeof(Expr)),
    .num_args = 2,
  };
  pair_expr.args[0] = (Expr){.type=SYM, .as.sym="a"};
  pair_expr.args[1] = (Expr){.type=SYM, .as.sym="b"};

  NamedExpr swapped_pair_expr = {
    .name = "pair",
    .args = calloc(2, sizeof(Expr)),
    .num_args = 2,
  };
  swapped_pair_expr.args[0] = (Expr){.type=SYM, .as.sym="b"};
  swapped_pair_expr.args[1] = (Expr){.type=SYM, .as.sym="a"};

  Func swap_functor = {
    .name = "swap",
    .head = calloc(1, sizeof(Expr)),
    .body = calloc(1, sizeof(Expr)),
  };
  swap_functor.head[0] = (Expr){
    .type = NAMED_EXPR,
    .as.named_expr = pair_expr,
  };
  swap_functor.body[0] = (Expr){
    .type = NAMED_EXPR,
    .as.named_expr = swapped_pair_expr,
  };

  NamedExpr first_ele = {
    .name = "f",
    .args = calloc(3, sizeof(Expr)),
    .num_args = 3
  };
  first_ele.args[0] = (Expr){.type=SYM, .as.sym="j"};
  first_ele.args[1] = (Expr){.type=SYM, .as.sym="k"};
  first_ele.args[2] = (Expr){.type=SYM, .as.sym="l"};

  NamedExpr second_ele = {
    .name = "g",
    .args = calloc(1, sizeof(Expr)),
    .num_args = 1
  };
  second_ele.args[0] = (Expr){.type=SYM, .as.sym="w"};

  NamedExpr input = {
    .name = "pair",
    .args = calloc(2, sizeof(Expr)),
    .num_args = 2,
  };
  input.args[0] = (Expr){.type=NAMED_EXPR, .as.named_expr=first_ele};
  input.args[1] = (Expr){.type=NAMED_EXPR, .as.named_expr=second_ele};

  Expr input_expr = wrap_in_expr(input);
  printf("Input to manipulate:    ");
  print_expr(input_expr);

  printf("Functor to apply:       ");
  print_expr(wrap_in_expr(swap_functor));

  SymMap sym_map = new_sym_map(16);
  if (match_exprs(input_expr, wrap_in_expr(swap_functor), &sym_map)) {
    printf("=========================================\n");
    printf("Result of application:  ");
    Expr result = execute_functor(input_expr, *swap_functor.head, *swap_functor.body, sym_map);
    print_expr(result);
  } else {
    printf("No match\n");
  }

  free_sym_map(&sym_map);
  free(input.args);
  free(pair_expr.args);
  free(swapped_pair_expr.args);
  free(swap_functor.head);
  free(swap_functor.body);

  return 0;
}
