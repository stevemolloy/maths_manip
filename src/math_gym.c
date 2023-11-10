#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <readline/readline.h>
#include <stdlib.h>
#include <string.h>

#include "gym_lib.h"

#define FUNCLIST_CAP 256

#define SM_add_to_array(arr, val)                                                      \
  do {                                                                                 \
       if ((arr)->length >= (arr)->capacity) {                                         \
         (arr)->capacity *= 2;                                                         \
         (arr)->data = realloc((arr)->data, (arr)->capacity * sizeof((arr)->data[0])); \
         if ((arr)->data == NULL) {                                                    \
            fprintf(stderr, "Could not allocate memory to extend the array.");    \
            exit(1);                                                                   \
         }                                                                             \
       }                                                                               \
       (arr)->data[(arr)->length++] = val;                                             \
  } while (0)

#define SM_free(arr) free((arr).data)

typedef struct {
  size_t length;
  size_t capacity;
  Func *definitions;
} FunctorList;

typedef struct {
  FunctorList functors;
  Expr expr;
} AppState;

FunctorList new_functorlist(size_t capacity) {
  FunctorList fl = {
    .length = 0,
    .capacity = capacity,
    .definitions = calloc(capacity, sizeof(Func)),
  };
  return fl;
}

int main(void) {
  AppState state = {
    .functors = new_functorlist(FUNCLIST_CAP)
  };

  // bool should_quit = false;
  char *line_read = NULL;

  // while (!should_quit) {
  line_read = readline ("Give an expression to manipulate: ");
  Expr input_expr = parse_cstring_to_expr(line_read);
  line_read = NULL;

  line_read = readline ("Give a functor to apply: ");
  Expr swap_functor = parse_cstring_to_expr(line_read);

  //   if (strcmp(line_read, "quit")==0 || strcmp(line_read, "exit")==0) {
  //     should_quit = true;
  //   }
  // }

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
