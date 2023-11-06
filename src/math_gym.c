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
    } else if (*cursor == '=' && *(cursor+1)!='\0' && *(cursor+1)=='>') {
      tok = new_token(FUNCTOR_EQ, "=>", 2);
      cursor++;
    }
    add_to_token_list(&token_list, tok);
    cursor++;
  }

  return token_list;
}

// Expr token_list_to_expr(TokenList tokens, size_t i) {
//   assert(i < tokens.len);
//   Token tok = tokens.tokens[i];
//   if (tok.type==WORD && (i == tokens.len-1 || tokens.tokens[i+1].type != LEFTPAREN)) {
//     Sym new_sym = calloc(strlen(tok.contents)+1, sizeof(char));
//     memcpy(new_sym, tok.contents, strlen(tok.contents)+1);
//     return wrap_in_expr(new_sym);
//   } else if (tok.type==WORD) {
//     char *name = calloc(strlen(tok.contents)+1, sizeof(char));
//     memcpy(name, tok.contents, strlen(tok.contents)+1);
//     i += 2;
//     size_t startpt = i;
//     tok = tokens.tokens[i];
//     size_t arg_count = 0;
//     while (tok.type != RIGHTPAREN) {
//       if (tok.type == WORD) arg_count++;
//       tok = tokens.tokens[++i];
//     }
//     NamedExpr ne = (NamedExpr) {
//       .name = name,
//       .args = calloc(arg_count, sizeof(Expr)),
//       .num_args = arg_count,
//     };
//     i = startpt;
//     tok = tokens.tokens[i];
//     arg_count = 0;
//     while (tok.type != RIGHTPAREN) {
//       if (tok.type == WORD) {
//         ne.args[arg_count] = token_list_to_expr(tokens, i);
//         arg_count++;
//       }
//       tok = tokens.tokens[++i];
//     }
//     if (i == tokens.len - 1)
//       return wrap_in_expr(ne);
//     
//     i++;
//     assert(tokens.tokens[i].type == FUNCTOR_EQ && 
//         "Did not find \"=>\" in the expected place");
//
//     i++;
//     Expr rhs = token_list_to_expr(tokens, i);
//     print_expr(rhs);
//   }
//   assert(0 && "Unreachable");
// }

Expr token_list_to_expr(TokenList tl, size_t *cursor) {
  assert(*cursor < tl.len); // A bit of memory protection

  assert(tl.len > 0);
  if (*cursor == tl.len - 1) {
    // Cursor points right to the end of the TokenList. If this isn't 
    // a WORD, then the input must be malformed.
    assert(tl.tokens[*cursor].type == WORD && "Malformed input");
    return wrap_in_expr(make_sym(tl.tokens[*cursor].contents));
  }

  // Due to the two previous assertions, we know that there is at least
  // one element after the cursor position. So "cursor+1" is a safe index.
  assert(*cursor < tl.len - 1);

  // What we need to do next depends on the type of the current token
  switch (tl.tokens[*cursor].type) {
    case WORD:
      // The following (cursor+1) is safe due to the previous asserts
      if (tl.tokens[*cursor+1].type == LEFTPAREN) {
        // This is a NAMED_EXPR or a FUNCTOR. In either case, it needs a "name".
        char* name = (char*) make_sym(tl.tokens[*cursor].contents);

        // This is a NAMED_EXPR or a FUNCTOR. Check for the latter by 
        // scanning for the matching RIGHTPAREN and seeing if a FUNCTOR_EQ is next
        bool is_functor = false;
        *cursor += 2; // The cursor now points at the token immediately after the LEFTPAREN
        int paren_balance = 1;  // The difference between num of LEFTPARENs and RIGHTPARENs
        size_t i = *cursor;  // Preserve the cursor position, and instead scan a throwaway variable
        for ( ; i<tl.len; i++) {
          if (tl.tokens[i].type == LEFTPAREN) paren_balance++;
          if (tl.tokens[i].type == RIGHTPAREN) paren_balance--;
          if (paren_balance == 0) break;
        }
        // At this point, we have found the matching rightparen and "i" points directly at it
        i++;
        if (i != tl.len && tl.tokens[i].type == FUNCTOR_EQ) is_functor = true;

        // At this point the cursor is still pointing at the token just after the LEFTPAREN
        if (is_functor) {
          // This is a functor. That means the next thing is a named_expression
          NamedExpr lhs = {0};
          assert(tl.tokens[*cursor].type == WORD);
          lhs.name = (char*)make_sym(tl.tokens[*cursor].contents);
          (*cursor)++;
          assert(tl.tokens[*cursor].type == LEFTPAREN);
          (*cursor)++;
          lhs.num_args = 0;
          while (tl.tokens[*cursor].type != RIGHTPAREN) {
            if (tl.tokens[*cursor].type == WORD) {
              lhs.num_args++;
              lhs.args = realloc(lhs.args, lhs.num_args * sizeof(Expr));
              lhs.args[lhs.num_args-1] = token_list_to_expr(tl, cursor);
            }
            (*cursor)++;
          }
          *cursor += 2;
          assert(tl.tokens[*cursor].type == FUNCTOR_EQ);

          (*cursor)++;
          Expr *rhs_expr = calloc(1, sizeof(Expr));
          *rhs_expr = token_list_to_expr(tl, cursor);

          Expr *lhs_expr = calloc(1, sizeof(Expr));
          *lhs_expr = wrap_in_expr(lhs);

          Func result = {
            .name = name,
            .head = lhs_expr,
            .body = rhs_expr,
          };

          return wrap_in_expr(result);
        } else {
          // This is a named expression
          NamedExpr ne = {0};
          ne.name = name;
          ne.num_args = 0;
          while (tl.tokens[*cursor].type != RIGHTPAREN) {
            if (tl.tokens[*cursor].type == WORD) {
              ne.num_args++;
              ne.args = realloc(ne.args, ne.num_args * sizeof(Expr));
              ne.args[ne.num_args-1] = token_list_to_expr(tl, cursor);
            }
            (*cursor)++;
          }
          if (tl.tokens[*cursor].type == RIGHTPAREN) {
            return wrap_in_expr(ne);
          }
        }
      } else {
        // This is a simple symbol.
        // I think this is necessary to allow the recursion to work
        return wrap_in_expr(make_sym(tl.tokens[*cursor].contents));
      }
			break;
    case LEFTPAREN:
    case RIGHTPAREN:
    case COMMA:
    case FUNCTOR_EQ:
      assert(0 && "Malformed input?");
  }

  TODO();
}

Expr parse_cstring_to_expr(char *input_string) {
  TokenList token_list = parse_input_string(input_string);
  size_t cursor = 0;
  Expr result = token_list_to_expr(token_list, &cursor);
  free_tokenlist(&token_list);
  return result;
}

int main(void) {
  Expr test = parse_cstring_to_expr("swap(pair(a, b)) => pair(b, a)");
  // Expr test = parse_cstring_to_expr("pair(a, b)");
  print_expr(test);
  return 1;

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
