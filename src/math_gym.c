#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gym_lib.h"

#define VERBOSE false

#define TODO() assert(0 && "Not yet implemented\n")

typedef enum {
  WORD,
  LEFTPAREN,
  RIGHTPAREN,
  COMMA,
  FUNCTOR_EQ,
} TokenType;

char* tokentype_to_cstring(TokenType tt) {
  switch (tt) {
    case WORD:
      return "WORD";
    case LEFTPAREN:
      return "LEFTPAREN";
    case RIGHTPAREN:
      return "RIGHTPAREN";
    case COMMA:
      return "COMMA";
    case FUNCTOR_EQ:
      return "FUNCTOR_EQ";
  }
}

typedef struct{
  TokenType type;
  char *contents;
} Token;

Token new_token(TokenType type, char *contents, size_t len) {
  Token result = {0};
  result.type = type;
  result.contents = calloc(len + 1, sizeof(char));
  memcpy(result.contents, contents, len);
  result.contents[len] = '\0';
  return result;
}

typedef struct {
  size_t len;
  size_t cap;
  Token *tokens;
} TokenList;

TokenList new_token_list(size_t ini_cap) {
  TokenList result = {
    .cap = ini_cap,
    .tokens = calloc(ini_cap, sizeof(Token)),
  };
  if (result.tokens == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    exit(1);
  }
  return result;
}

int add_to_token_list(TokenList *tl, Token token) {
  if (tl->len >= tl->cap) {
    tl->tokens = realloc(tl->tokens, tl->cap * 2 * sizeof(Token));
    if (tl->tokens == NULL) {
      fprintf(stderr, "Memory allocation error\n");
      return -1;
    }
    tl->cap *= 2;
  }
  tl->tokens[tl->len++] = token;
  return 1;
}

TokenList parse_input_string(char *input_string) {
  TokenList token_list = new_token_list(32);
  char *cursor = input_string;
  
  while (*cursor != '\0') {
    Token tok = {0};
    if (isspace(*cursor)) {
      cursor++;
      continue;
    }
    else if (isalpha(*cursor)) {
      char *token_cursor = cursor;
      size_t len = 0;
      while (isalpha(*token_cursor) != 0) {
        len++;
        token_cursor++;
      }
      tok = new_token(WORD, cursor, len);
      cursor = token_cursor - 1;
    } else if (*cursor == '(') {
      tok = new_token(LEFTPAREN, cursor, 1);
    } else if (*cursor == ')') {
      tok = new_token(RIGHTPAREN, cursor, 1);
    } else if (*cursor == ',') {
      tok = new_token(COMMA, cursor, 2);
    }
    add_to_token_list(&token_list, tok);
    cursor++;
  }

  return token_list;
}

int main(void) {
  char *pair_expr_str = "pair(a, b)";

  TokenList token_list = parse_input_string(pair_expr_str);

  for (size_t i=0; i<token_list.len; i++) {
    printf(
        "%zu: (%s) %s\n",
        i,
        tokentype_to_cstring(token_list.tokens[i].type),
        token_list.tokens[i].contents
      );
  }

  return 1;

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
