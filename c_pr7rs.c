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
        if (!fp)
            return NULL;
    }

    char *buf;
    size_t len;
    FILE *out = open_memstream(&buf, &len);

    for (;;) {
        char buf2[2048];
        int n = fread(buf2, 1, sizeof(buf2), fp);
        if (n == 0)
            break;
        fwrite(buf2, 1, n, out);
    }

    if (fp != stdin)
        fclose(fp);

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
            if (*c == '\n')
                in_comment = false;
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
            if (*c == 't')
                type = TOK_TRUE;
            tok->next = token(type, c - 1, 2);
            c++;
        } else if (*c == '\'') {
            tok->next = token(TOK_QUOTE, c, 1);
            c++;
        } else if (isdigit(*c)) {
            // TODO: support +10, -2
            char *start = c;
            while (isdigit(*c))
                c++;
            tok->next = token(TOK_NUM, start, c - start);
        } else if (isalpha(*c) || strchr(ident_chars, *c)) {
            char *start = c;
            while (*c != '\0' && (isalnum(*c) || strchr(ident_chars, *c)))
                c++;
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
    case TOK_QUOTE:
        return parse(&(*tok)->next); // TODO: fix
    }
}

// Obj *eval(Obj *obj) {
//     // TODO: env
//     switch (obj->type) {
//         case OBJ_NUM:
//         case OBJ_BOOL:
//             return obj;
//         case OBJ_SYM: {
//             char str[TOKEN_MAX_LEN];
//             strncpy(str, obj->value.symbol, obj->len);
//             obj->value.symbol = str;
//             return obj;
//         }
//         case OBJ_PAIR, OBJ_NIL } ObjType;
//     }
// }

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
    case OBJ_PAIR:
        printf("( ");
        print_obj_2(obj->value.pair.car);
        printf(" . ");
        print_obj_2(obj->value.pair.cdr);
        printf(" )");
        break;
    case OBJ_NIL:
        printf("nil");
        break;
    }
}

void print_obj(Obj *obj) {
    printf("[DEBUG] parse result:\n");
    print_obj_2(obj);
    printf("\n");
}

int main(int argc, char *argv[]) {
    // char *lines = read_file("");
    char *input = argv[1];
    Token *tok = tokenize(input);
    print_tokens(tok);
    Obj *o = parse(&tok);
    print_obj(o);
    return 0;
}
