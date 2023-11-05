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

void free_tokenlist(TokenList *token_list) {
  for (size_t i=0; i<token_list->len; i++) {
    free(token_list->tokens[i].contents);
  }
  free(token_list->tokens);
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

// typedef struct AST AST; // Forward reference
//
// struct AST {
//   enum {
//     AST_SYMBOL,
//     AST_NAMED_EXPR,
//   } tag;
//   union {
//     struct AST_SYMBOL { Sym sym; } AST_SYMBOL;
//     struct AST_NAMED_EXPR { AST *ast_list; } AST_NAMED_EXPR;
//   } as;
// };
//
// AST *ast_new(AST ast) {
//   AST *ptr = malloc(sizeof(AST));
//   if (ptr) *ptr = ast;
//   return ptr;
// }
//
// AST tokenlist_to_ast(TokenList token_list, size_t cursor) {
//   switch (token_list.tokens[cursor].type) {
//     case WORD:
//       if (cursor == token_list.len-1 || token_list.tokens[cursor].type != LEFTPAREN) {
//         return (AST) {
//           .tag = AST_SYMBOL,
//           .as.AST_SYMBOL = (Sym)token_list.tokens[cursor].contents,
//         };
//       } else {
//         return (AST) [
//           .tag = AST_NAMED_EXPR,
//           .as.AST_NAMED_EXPR = 
//         ]
//       }
//   }
// }

Expr token_list_to_expr(TokenList tokens, size_t i) {
  assert(i < tokens.len);
  Token tok = tokens.tokens[i];
  if (tok.type==WORD && (i == tokens.len-1 || tokens.tokens[i+1].type != LEFTPAREN)) {
    Sym new_sym = calloc(strlen(tok.contents)+1, sizeof(char));
    memcpy(new_sym, tok.contents, strlen(tok.contents)+1);
    return wrap_in_expr(new_sym);
  } else if (tok.type==WORD) {
    char *name = calloc(strlen(tok.contents)+1, sizeof(char));
    memcpy(name, tok.contents, strlen(tok.contents)+1);
    i += 2;
    size_t startpt = i;
    tok = tokens.tokens[i];
    size_t arg_count = 0;
    while (tok.type != RIGHTPAREN) {
      if (tok.type == WORD) arg_count++;
      tok = tokens.tokens[++i];
    }
    NamedExpr ne = (NamedExpr) {
      .name = name,
      .args = calloc(arg_count, sizeof(Expr)),
      .num_args = arg_count,
    };
    i = startpt;
    tok = tokens.tokens[i];
    arg_count = 0;
    while (tok.type != RIGHTPAREN) {
      if (tok.type == WORD) {
        ne.args[arg_count] = token_list_to_expr(tokens, i);
        arg_count++;
      }
      tok = tokens.tokens[++i];
    }
    return wrap_in_expr(ne);
  }
  assert(0 && "Unreachable");
}

Expr parse_cstring_to_expr(char *input_string) {
  TokenList token_list = parse_input_string(input_string);
  Expr result = token_list_to_expr(token_list, 0);
  free_tokenlist(&token_list);
  return result;
}

int main(void) {
  Func swap_functor = {
    .name = "swap",
    .head = calloc(1, sizeof(Expr)),
    .body = calloc(1, sizeof(Expr)),
  };
  swap_functor.head[0] = parse_cstring_to_expr("pair(a, b)");
  swap_functor.body[0] = parse_cstring_to_expr("pair(b, a)");

  Expr input_expr = parse_cstring_to_expr("pair(x, y)");

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

  for (size_t i=0; i<swap_functor.head->as.named_expr.num_args; i++) {
    free(swap_functor.head->as.named_expr.args[i].as.sym);
  }
  free(swap_functor.head->as.named_expr.args);
  free(swap_functor.head->as.named_expr.name);
  free(swap_functor.head);
  for (size_t i=0; i<swap_functor.body->as.named_expr.num_args; i++) {
    free(swap_functor.body->as.named_expr.args[i].as.sym);
  }
  free(swap_functor.body->as.named_expr.args);
  free(swap_functor.body->as.named_expr.name);
  free(swap_functor.body);

  for (size_t i=0; i<input_expr.as.named_expr.num_args; i++) {
    free(input_expr.as.named_expr.args[i].as.sym);
  }
  free(input_expr.as.named_expr.args);
  free(input_expr.as.named_expr.name);

  return 0;
}
