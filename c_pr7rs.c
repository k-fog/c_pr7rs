#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKEN_MAX_LEN 32
#define ENV_SIZE (64 + 3)

const char *ident_chars = "!$%%&*+-./:<=>?^_~";

typedef struct Obj Obj;
typedef struct Env Env;
typedef struct Procedure Procedure;

typedef enum ObjType {
    OBJ_NUM,
    OBJ_BOOL,
    OBJ_SYM,
    OBJ_PAIR,
    OBJ_PRIM,
    OBJ_PROC,
    OBJ_NIL
} ObjType;

struct Obj {
    ObjType type;
    size_t len;
    union {
        int number;
        bool boolean;
        char *symbol;
        struct Pair {
            Obj *car;
            Obj *cdr;
        } pair;
        Obj *(*prim_fn)(Obj *args, Env *env);
        Procedure *proc;
    };
};

Obj *obj(ObjType type) {
    Obj *obj = calloc(1, sizeof(Obj));
    obj->type = type;
    return obj;
}

Obj *number(int n) {
    Obj *o = obj(OBJ_NUM);
    o->number = n;
    return o;
}

Obj *symbol(char *sym) {
    Obj *o = obj(OBJ_SYM);
    o->symbol = sym;
    o->len = strlen(sym);
    return o;
}

Obj *prim_fn(Obj *(*fn)(Obj *args, struct Env *env)) {
    Obj *o = obj(OBJ_PRIM);
    o->prim_fn = fn;
    return o;
}

Obj *boolean(bool b) {
    Obj *o = obj(OBJ_BOOL);
    o->boolean = b;
    return o;
}

struct Procedure {
    Obj *params;
    Obj *body;
    Env *outer_env;
};

Obj *procedure(Obj *params, Obj *body, Env *outer_env) {
    Procedure *p = calloc(1, sizeof(Procedure));
    p->params = params;
    p->body = body;
    p->outer_env = outer_env;
    Obj *o = obj(OBJ_PROC);
    o->proc = p;
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

typedef struct Entry {
    Obj *key;
    Obj *value;
    struct Entry *next;
} Entry;

typedef struct Env {
    Entry **vars;
    struct Env *outer;
} Env;

Entry *entry(Obj *key, Obj *value) {
    Entry *e = calloc(1, sizeof(Entry));
    e->key = key;
    e->value = value;
    return e;
}

Env *make_env(Env *outer) {
    Env *e = calloc(1, sizeof(Env));
    e->vars = calloc(ENV_SIZE, sizeof(Entry));
    for (size_t i = 0; i < ENV_SIZE; i++) {
        e->vars[i] = NULL;
    }
    e->outer = outer;
    return e;
}

int hash(const char *key, size_t key_len) {
    int hash = 0;
    for (int i = 0; i < key_len; i++) {
        hash ^= (key[i] << (i % 4) * 8); 
    }
    return hash % ENV_SIZE;
}

void push_env(Env *env, Obj *key, Obj *value) {
    int idx = hash(key->symbol, key->len);
    Entry *e = env->vars[idx];
    while (e != NULL) {
        if (e->key->len == key->len && strncmp(e->key->symbol, key->symbol, key->len) == 0) {
            printf("error\n");
        }
        e = e->next;
    }
    e = entry(key, value);
    e->next = env->vars[idx];
    env->vars[idx] = e;
    return;
}

Obj *find(Env *env, Obj *key) {
    int idx = hash(key->symbol, key->len);
    Entry *e = env->vars[idx];
    while (e != NULL) {
        if (strncmp(e->key->symbol, key->symbol, key->len) == 0) {
            return e->value;
        }
        e = e->next;
    }
    return env->outer == NULL ? NULL : find(env->outer, key);
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
                cur_obj->pair.car = parse(tok);
                cur_obj->pair.cdr = obj(OBJ_PAIR);
                *tok = (*tok)->next;
                cur_obj = cur_obj->pair.cdr;
            }
            cur_obj->type = OBJ_NIL;
            return head;
        }
        case TOK_RPAREN:
            return NULL;
        case TOK_NUM: {
            Obj *o = obj(OBJ_NUM);
            o->number = strtol((*tok)->str, NULL, 10);
            return o;
        }
        case TOK_IDENT: {
            Obj *o = obj(OBJ_SYM);
            o->symbol = (*tok)->str;
            o->len = (*tok)->len;
            return o;
        }
        case TOK_TRUE: {
            Obj *o = obj(OBJ_BOOL);
            o->boolean = true;
            return o;
        }
        case TOK_FALSE: {
            Obj *o = obj(OBJ_BOOL);
            o->boolean = false;
            return o;
        }
        case TOK_QUOTE: {
            Obj *o = obj(OBJ_PAIR);
            o->pair.car = symbol("quote");
            o->pair.cdr = obj(OBJ_PAIR);
            o->pair.cdr->pair.car = parse(&(*tok)->next);
            o->pair.cdr->pair.cdr = obj(OBJ_NIL);
            return o;
        }
    }
}

bool is_truthy(Obj *obj) {
    return obj->type != OBJ_BOOL || obj->boolean == true;
}

Obj *car(Obj *cons) { return cons->pair.car; }

Obj *cdr(Obj *cons) { return cons->pair.cdr; }

Obj *eval(Obj *obj, Env *env);
Obj *eval_2(Obj *obj, Env *env, bool is_quoted);

Obj *f_add(Obj *args, Env *env) {
    int n = 0;
    while (args->type == OBJ_PAIR) {
        n += eval(car(args), env)->number;
        args = cdr(args);
    }
    return number(n);
}

Obj *f_sub(Obj *args, Env *env) {
    int n = eval(car(args), env)->number;
    args = cdr(args);
    while (args->type == OBJ_PAIR) {
        n -= eval(car(args), env)->number;
        args = cdr(args);
    }
    return number(n);
}

Obj *f_mult(Obj *args, Env *env) {
    int n = eval(car(args), env)->number;
    args = cdr(args);
    while (args->type == OBJ_PAIR) {
        n *= eval(car(args), env)->number;
        args = cdr(args);
    }
    return number(n);
}

Obj *f_if(Obj *args, Env *env) {
    Obj *ret = eval(car(args), env);
    return is_truthy(ret) ? eval(car(cdr(args)), env) : eval(car(cdr(cdr(args))), env);
}

Obj *f_quote(Obj *args, Env *env) { return eval_2(car(args), env, true); }

Obj *f_define(Obj *args, Env *env) {
    Obj *key = car(args);
    Obj *value = eval(car(cdr(args)), env);
    push_env(env, key, value);
    return symbol("Undefined");
}

Obj *f_lambda(Obj *args, Env *env) {
    Obj *params = car(args);
    Obj *body = car(cdr(args));
    return procedure(params, body, env);
}

Obj *f_boolp(Obj *args, Env *env) {
    return eval(car(args), env)->type == OBJ_BOOL ? boolean(true) : boolean(false);
}

Obj *f_numberp(Obj *args, Env *env) {
    return eval(car(args), env)->type == OBJ_NUM ? boolean(true) : boolean(false);
}

Obj *f_procp(Obj *args, Env *env) {
    Obj *ret = eval(car(args), env);
    return ret->type == OBJ_PROC || ret->type == OBJ_PRIM ? boolean(true)
                                                          : boolean(false);
}

Obj *f_nullp(Obj *args, Env *env) {
    return eval(car(args), env)->type == OBJ_NIL ? boolean(true) : boolean(false);
}

Obj *f_pairp(Obj *args, Env *env) {
    return eval(car(args), env)->type == OBJ_PAIR ? boolean(true) : boolean(false);
}

Obj *f_symbolp(Obj *args, Env *env) {
    return eval(car(args), env)->type == OBJ_SYM ? boolean(true) : boolean(false);
}

Obj *f_car(Obj *args, Env *env) {
    return eval(car(args), env)->pair.car;
}

Obj *f_cdr(Obj *args, Env *env) {
    return eval(car(args), env)->pair.cdr;
}

Obj *f_not(Obj *args, Env *env) {
    return is_truthy(eval(car(args), env)) ? boolean(false) : boolean(true);
}

Obj *apply(Obj *fn, Obj *args, Env *env) {
    if (fn->type == OBJ_PRIM)
        return fn->prim_fn(args, env);
    else if (fn->type == OBJ_PROC) {
        Env *new_env = make_env(fn->proc->outer_env);
        Obj *cur_params = fn->proc->params;
        Obj *cur_args = args;
        while (cur_params->type == OBJ_PAIR) {
            push_env(new_env, car(cur_params), car(cur_args));
            cur_params = cdr(cur_params);
            cur_args = cdr(cur_args);
        }
        return eval(fn->proc->body, new_env);
    }
    printf("error\n");
    return NULL;
}

Obj *eval(Obj *obj, Env *env) { return eval_2(obj, env, false); }

Obj *eval_2(Obj *obj, Env *env, bool is_quoted) {
    if (is_quoted) return obj;
    switch (obj->type) {
        case OBJ_NUM:
        case OBJ_BOOL:
        case OBJ_NIL:
        case OBJ_PRIM:
        case OBJ_PROC:
            return obj;
        case OBJ_SYM:
            return find(env, obj);
            ;
        case OBJ_PAIR: {
            Obj *fn = eval(car(obj), env);
            Obj *args = cdr(obj);
            return apply(fn, args, env);
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
            printf("%d", obj->number);
            break;
        case OBJ_BOOL:
            printf("%s", obj->boolean ? "#t" : "#f");
            break;
        case OBJ_SYM: {
            char sym_str[TOKEN_MAX_LEN];
            strncpy(sym_str, obj->symbol, obj->len);
            sym_str[obj->len] = '\0';
            printf("%s", sym_str);
            break;
        }
        case OBJ_PRIM:
            printf("<function>");
            break;
        case OBJ_PROC:
            printf("<procedure>");
            break;
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

void add_prims(Env *env) {
    push_env(env, symbol("+"), prim_fn(f_add));
    push_env(env, symbol("-"), prim_fn(f_sub));
    push_env(env, symbol("*"), prim_fn(f_mult));
    push_env(env, symbol("if"), prim_fn(f_if));
    push_env(env, symbol("quote"), prim_fn(f_quote));
    push_env(env, symbol("define"), prim_fn(f_define));
    push_env(env, symbol("lambda"), prim_fn(f_lambda));
    push_env(env, symbol("boolean?"), prim_fn(f_boolp));
    push_env(env, symbol("number?"), prim_fn(f_numberp));
    push_env(env, symbol("procedure?"), prim_fn(f_procp));
    push_env(env, symbol("null?"), prim_fn(f_nullp));
    push_env(env, symbol("pair?"), prim_fn(f_pairp));
    push_env(env, symbol("symbol?"), prim_fn(f_symbolp));
    push_env(env, symbol("car"), prim_fn(f_car));
    push_env(env, symbol("cdr"), prim_fn(f_cdr));
    push_env(env, symbol("not"), prim_fn(f_not));
}

int main(int argc, char *argv[]) {
    // char *lines = read_file("");
    char *input = argv[1];
    Token *tok = tokenize(input);
    // print_tokens(tok);
    Obj *ast = parse(&tok);
    // print_obj(ast);
    Env *global_env = make_env(NULL);
    add_prims(global_env);
    Obj *result = eval(ast, global_env);
    print_obj(result);
    return 0;
}
