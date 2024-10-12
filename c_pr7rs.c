#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKEN_MAX_LEN 32

const char *ident_chars = "!$%%&*+-./:<=>?^_~";

typedef enum ObjType { OBJ_NUM, OBJ_BOOL, OBJ_SYM, OBJ_PAIR, OBJ_NIL } ObjType;
typedef struct Obj {
    ObjType type;
    union {
        int number;
        bool boolean;
        char *symbol;
        struct Pair {
            struct Obj *car;
            struct Obj *cdr;
        } pair;
    } value;
    size_t len;
} Obj;

Obj *obj(ObjType type) {
    Obj *obj = calloc(1, sizeof(Obj));
    obj->type = type;
    return obj;
}

Obj *number(int n) {
    Obj *o = obj(OBJ_NUM);
    o->value.number = n;
    return o;
}

Obj *symbol(char *sym) {
    Obj *o = obj(OBJ_SYM);
    o->value.symbol = sym;
    o->len = strlen(sym);
    return o;
}

typedef enum {
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_NUM,
    TOK_IDENT,
    TOK_TRUE,
    TOK_FALSE,
    TOK_QUOTE
} TokenType;
typedef struct Token {
    TokenType type;
    char *str;
    size_t len;
    struct Token *next;
} Token;

Token *token(TokenType type, char *str, size_t len) {
    Token *t = calloc(1, sizeof(Token));
    t->type = type;
    t->str = str;
    t->len = len;
    t->next = NULL;
    return t;
}

char *read_file(char *path) {
    FILE *fp;
    if (strcmp(path, "") == 0) {
        fp = stdin;
    } else {
        fp = fopen(path, "r");
        if (!fp) return NULL;
    }

    char *buf;
    size_t len;
    FILE *out = open_memstream(&buf, &len);

    for (;;) {
        char buf2[2048];
        int n = fread(buf2, 1, sizeof(buf2), fp);
        if (n == 0) break;
        fwrite(buf2, 1, n, out);
    }

    if (fp != stdin) fclose(fp);

    fflush(out);
    fputc('\0', out);
    fclose(out);
    return buf;
}

Token *tokenize(char *c) {
    Token *head = token(-1, NULL, -1);
    Token *tok = head;
    bool in_comment = false;
    while (*c != '\0') {
        // skip comment
        if (in_comment) {
            if (*c == '\n') in_comment = false;
            c++;
            continue;
        }
        // skip space
        if (isspace(*c)) {
            c++;
            continue;
        }
        // detect comment
        else if (*c == ';') {
            in_comment = true;
            c++;
            continue;
        } else if (*c == '(') {
            tok->next = token(TOK_LPAREN, c, 1);
            c++;
        } else if (*c == ')') {
            tok->next = token(TOK_RPAREN, c, 1);
            c++;
        } else if (*c == '#') {
            TokenType type = TOK_FALSE;
            c++;
            if (*c == 't') type = TOK_TRUE;
            tok->next = token(type, c - 1, 2);
            c++;
        } else if (*c == '\'') {
            tok->next = token(TOK_QUOTE, c, 1);
            c++;
        } else if (isdigit(*c)) {
            // TODO: support +10, -2
            char *start = c;
            while (isdigit(*c)) c++;
            tok->next = token(TOK_NUM, start, c - start);
        } else if (isalpha(*c) || strchr(ident_chars, *c)) {
            char *start = c;
            while (*c != '\0' && (isalnum(*c) || strchr(ident_chars, *c))) c++;
            tok->next = token(TOK_IDENT, start, c - start);
        } else {
            c++;
            continue;
        }

        tok = tok->next;
    }
    return head->next;
}

Obj *parse(Token **tok) {
    TokenType type = (*tok)->type;
    switch (type) {
        case TOK_LPAREN: {
            Obj *cur_obj = obj(OBJ_PAIR);
            Obj *head = cur_obj;
            *tok = (*tok)->next;
            while ((*tok)->type != TOK_RPAREN) {
                cur_obj->value.pair.car = parse(tok);
                cur_obj->value.pair.cdr = obj(OBJ_PAIR);
                *tok = (*tok)->next;
                cur_obj = cur_obj->value.pair.cdr;
            }
            cur_obj->type = OBJ_NIL;
            return head;
        }
        case TOK_RPAREN:
            return NULL;
        case TOK_NUM: {
            Obj *o = obj(OBJ_NUM);
            o->value.number = strtol((*tok)->str, NULL, 10);
            return o;
        }
        case TOK_IDENT: {
            Obj *o = obj(OBJ_SYM);
            o->value.symbol = (*tok)->str;
            o->len = (*tok)->len;
            return o;
        }
        case TOK_TRUE: {
            Obj *o = obj(OBJ_BOOL);
            o->value.boolean = true;
            return o;
        }
        case TOK_FALSE: {
            Obj *o = obj(OBJ_BOOL);
            o->value.boolean = false;
            return o;
        }
        case TOK_QUOTE: {
            Obj *o = obj(OBJ_PAIR);
            o->value.pair.car = symbol("quote");
            o->value.pair.cdr = obj(OBJ_PAIR);
            o->value.pair.cdr->value.pair.car = parse(&(*tok)->next);
            o->value.pair.cdr->value.pair.cdr = obj(OBJ_NIL);
            return o;
        }
    }
}

Obj *car(Obj *cons) { return cons->value.pair.car; }

Obj *cdr(Obj *cons) { return cons->value.pair.cdr; }

Obj *eval(Obj *obj, bool is_quoted);

Obj *f_add(Obj *args) {
    int n = 0;
    while (args->type == OBJ_PAIR) {
        n += eval(car(args), false)->value.number;
        args = cdr(args);
    }
    return number(n);
}

Obj *f_sub(Obj *args) {
    int n = eval(car(args), false)->value.number;
    args = cdr(args);
    while (args->type == OBJ_PAIR) {
        n -= eval(car(args), false)->value.number;
        args = cdr(args);
    }
    return number(n);
}

Obj *f_mult(Obj *args) {
    int n = eval(car(args), false)->value.number;
    args = cdr(args);
    while (args->type == OBJ_PAIR) {
        n *= eval(car(args), false)->value.number;
        args = cdr(args);
    }
    return number(n);
}

Obj *f_if(Obj *args) {
    return eval(car(args), false)->value.boolean ? eval(car(cdr(args)), false)
                                          : eval(car(cdr(cdr(args))), false);
}

Obj *f_quote(Obj *args) {
    return eval(car(args), true);
}

Obj *apply(Obj *fn, Obj *args) {
    if (fn->type == OBJ_SYM) {
        if (fn->len == 1) {
            if (strncmp(fn->value.symbol, "+", 1) == 0)
                return f_add(args);
            else if (strncmp(fn->value.symbol, "-", 1) == 0)
                return f_sub(args);
            else if (strncmp(fn->value.symbol, "*", 1) == 0)
                return f_mult(args);
        } else if (fn->len == 2) {
            if (strncmp(fn->value.symbol, "if", 2) == 0) return f_if(args);
        } else if (fn->len == 5) {
            if (strncmp(fn->value.symbol, "quote", 5) == 0) return f_quote(args);
        }
    }
    printf("error\n");
    return NULL;
}

Obj *eval(Obj *obj, bool is_quoted) {
    // TODO: env
    if (is_quoted) return obj;
    switch (obj->type) {
        case OBJ_NUM:
        case OBJ_BOOL:
        case OBJ_NIL:
        case OBJ_SYM:
            return obj;
        case OBJ_PAIR: {
            Obj *fn = eval(car(obj), false);
            Obj *args = cdr(obj);
            return apply(fn, args);
        }
    }
}

void print_tokens(Token *tok_head) {
    printf("[DEBUG] tokens:\n");
    Token *tok = tok_head;
    while (tok != NULL) {
        char tok_str[TOKEN_MAX_LEN];
        strncpy(tok_str, tok->str, tok->len);
        tok_str[tok->len] = '\0';
        printf("%s %zu %d\n", tok_str, tok->len, tok->type);
        tok = tok->next;
    }
    return;
}

void print_obj_2(Obj *obj) {
    switch (obj->type) {
        case OBJ_NUM:
            printf("%d", obj->value.number);
            break;
        case OBJ_BOOL:
            printf("%s", obj->value.boolean ? "#t" : "#f");
            break;
        case OBJ_SYM: {
            char sym_str[TOKEN_MAX_LEN];
            strncpy(sym_str, obj->value.symbol, obj->len);
            sym_str[obj->len] = '\0';
            printf("%s", sym_str);
            break;
        }
        case OBJ_PAIR: {
            printf("(");
            print_obj_2(car(obj));
            Obj *cur_cdr = cdr(obj);
            bool done = false;
            while (!done) {
                if (cur_cdr->type == OBJ_PAIR) {
                    printf(" ");
                    print_obj_2(car(cur_cdr));
                    cur_cdr = cdr(cur_cdr);
                } else if (cur_cdr->type == OBJ_NIL) {
                    printf(")");
                    done = true;
                }
            }
            break;
        }
        case OBJ_NIL:
            printf("()");
            break;
    }
}

void print_obj(Obj *obj) {
    print_obj_2(obj);
    printf("\n");
}

int main(int argc, char *argv[]) {
    // char *lines = read_file("");
    char *input = argv[1];
    Token *tok = tokenize(input);
    // print_tokens(tok);
    Obj *ast = parse(&tok);
    // print_obj(ast);
    Obj *result = eval(ast, false);
    print_obj(result);
    return 0;
}
