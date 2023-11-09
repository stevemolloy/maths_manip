#include <string.h>

#include "gym_lib.h"

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

Token new_token(TokenType type, char *contents, size_t len) {
  Token result = {0};
  result.type = type;
  result.contents = calloc(len + 1, sizeof(char));
  memcpy(result.contents, contents, len);
  result.contents[len] = '\0';
  return result;
}

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

