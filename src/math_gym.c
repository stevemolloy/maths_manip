#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "gym_lib.h"

int main(void) {
  // Expr swap_functor = parse_cstring_to_expr("collapse(pair(a, a)) => a");
  Expr swap_functor = parse_cstring_to_expr("swap(pair(a, b)) => pair(b, a)");

  Expr input_expr = parse_cstring_to_expr("pair(x, y)");

  printf("Input to manipulate:    ");
  print_expr(input_expr);

  printf("Functor to apply:       ");
  print_expr(swap_functor);

  SymMap sym_map = new_sym_map(16);
  Expr result = {0};
  if (match_exprs(input_expr, swap_functor, &sym_map)) {
    printf("=========================================\n");
    printf("Result of application:  ");
    result = execute_functor(*swap_functor.as.func.body, sym_map);
    print_expr(result);
  } else {
    printf("No match\n");
  }

  // Free allocated memory
  // TODO: This needs to be done in a more user-friendly way
  free_sym_map(&sym_map);
  free_expr(&swap_functor);
  free(input_expr.as.named_expr.args);
  free(input_expr.as.named_expr.name);

  return 0;
}
