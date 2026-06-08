#include "lexer.h"
#include "../include/common.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int current_line = 1;
int has_lex_error = 0;
Token current_token;

static const char *source;
static int pos;

// Token统计
static int token_count_by_type[256];

// 关键字映射表（小写）
static const struct {
    const char *word;
    TokenType type;
} keywords[] = {
    {"const",     TOK_CONST},
    {"var",       TOK_VAR},
    {"procedure", TOK_PROCEDURE},
    {"if",        TOK_IF},
    {"then",      TOK_THEN},
    {"while",     TOK_WHILE},
    {"do",        TOK_DO},
    {"call",      TOK_CALL},
    {"read",      TOK_READ},
    {"write",     TOK_WRITE},
    {"begin",     TOK_BEGIN},
    {"end",       TOK_END},
    {"odd",       TOK_ODD},
    {NULL,        TOK_ERROR}
};

static int is_keyword(const char *word, TokenType *type) {
    int i;
    for (i = 0; keywords[i].word != NULL; i++) {
        if (strcmp(word, keywords[i].word) == 0) {
            *type = keywords[i].type;
            return 1;
        }
    }
    return 0;
}

void init_lexer(const char *src) {
    source = src;
    pos = 0;
    current_line = 1;
    has_lex_error = 0;
    memset(token_count_by_type, 0, sizeof(token_count_by_type));
}

void print_token_stats(void) {
    printf("\n========== 词法分析统计 ==========\n");
    if (token_count_by_type[TOK_CONST])     printf("保留字 const: %d\n", token_count_by_type[TOK_CONST]);
    if (token_count_by_type[TOK_VAR])       printf("保留字 var: %d\n", token_count_by_type[TOK_VAR]);
    if (token_count_by_type[TOK_PROCEDURE]) printf("保留字 procedure: %d\n", token_count_by_type[TOK_PROCEDURE]);
    if (token_count_by_type[TOK_IF])        printf("保留字 if: %d\n", token_count_by_type[TOK_IF]);
    if (token_count_by_type[TOK_THEN])      printf("保留字 then: %d\n", token_count_by_type[TOK_THEN]);
    if (token_count_by_type[TOK_WHILE])     printf("保留字 while: %d\n", token_count_by_type[TOK_WHILE]);
    if (token_count_by_type[TOK_DO])        printf("保留字 do: %d\n", token_count_by_type[TOK_DO]);
    if (token_count_by_type[TOK_CALL])      printf("保留字 call: %d\n", token_count_by_type[TOK_CALL]);
    if (token_count_by_type[TOK_READ])      printf("保留字 read: %d\n", token_count_by_type[TOK_READ]);
    if (token_count_by_type[TOK_WRITE])     printf("保留字 write: %d\n", token_count_by_type[TOK_WRITE]);
    if (token_count_by_type[TOK_BEGIN])     printf("保留字 begin: %d\n", token_count_by_type[TOK_BEGIN]);
    if (token_count_by_type[TOK_END])       printf("保留字 end: %d\n", token_count_by_type[TOK_END]);
    if (token_count_by_type[TOK_ODD])       printf("保留字 odd: %d\n", token_count_by_type[TOK_ODD]);
    if (token_count_by_type[TOK_IDENT])     printf("标识符: %d\n", token_count_by_type[TOK_IDENT]);
    if (token_count_by_type[TOK_NUMBER])    printf("无符号整数: %d\n", token_count_by_type[TOK_NUMBER]);
    printf("运算符+界符: %d\n",
        token_count_by_type[TOK_PLUS] + token_count_by_type[TOK_MINUS] +
        token_count_by_type[TOK_MUL] + token_count_by_type[TOK_DIV] +
        token_count_by_type[TOK_ASSIGN] + token_count_by_type[TOK_EQ] +
        token_count_by_type[TOK_NEQ] + token_count_by_type[TOK_LT] +
        token_count_by_type[TOK_LE] + token_count_by_type[TOK_GT] +
        token_count_by_type[TOK_GE] + token_count_by_type[TOK_COMMA] +
        token_count_by_type[TOK_SEMI] + token_count_by_type[TOK_PERIOD] +
        token_count_by_type[TOK_LPAREN] + token_count_by_type[TOK_RPAREN]);
    printf("================================\n\n");
}

// 跳过空白和注释，返回0表示到达EOF
static int skip_whitespace_and_comments(void) {
    while (source[pos] != '\0') {
        // 跳过空白字符
        if (source[pos] == ' ' || source[pos] == '\t' || source[pos] == '\r') {
            pos++;
            continue;
        }
        // 换行
        if (source[pos] == '\n') {
            pos++;
            current_line++;
            continue;
        }
        // 单行注释 //
        if (source[pos] == '/' && source[pos + 1] == '/') {
            pos += 2;
            while (source[pos] != '\0' && source[pos] != '\n') {
                pos++;
            }
            continue;
        }
        // 多行注释 /* ... */
        if (source[pos] == '/' && source[pos + 1] == '*') {
            pos += 2;
            while (source[pos] != '\0') {
                if (source[pos] == '*' && source[pos + 1] == '/') {
                    pos += 2;
                    break;
                }
                if (source[pos] == '\n') current_line++;
                pos++;
            }
            if (source[pos] == '\0') {
                printf("[词法错误] 多行注释未闭合 (行号:%d)\n", current_line);
                has_lex_error = 1;
            }
            continue;
        }
        // PL/0风格注释 (* ... *)
        if (source[pos] == '(' && source[pos + 1] == '*') {
            pos += 2;
            while (source[pos] != '\0') {
                if (source[pos] == '*' && source[pos + 1] == ')') {
                    pos += 2;
                    break;
                }
                if (source[pos] == '\n') current_line++;
                pos++;
            }
            if (source[pos] == '\0') {
                printf("[词法错误] 多行注释未闭合 (行号:%d)\n", current_line);
                has_lex_error = 1;
            }
            continue;
        }
        break;
    }
    return source[pos] != '\0';
}

void next_token(void) {
    if (!skip_whitespace_and_comments()) {
        current_token.type = TOK_EOF;
        current_token.line_no = current_line;
        current_token.value = 0;
        strcpy(current_token.lexeme, "$");
        return;
    }

    current_token.line_no = current_line;
    current_token.value = 0;
    memset(current_token.lexeme, 0, MAX_TOKEN_LEN);

    char ch = source[pos];

    // 标识符或关键字（字母或下划线开头）
    if (isalpha(ch) || ch == '_') {
        int i = 0;
        while (isalnum((unsigned char)source[pos]) || source[pos] == '_') {
            if (i < MAX_TOKEN_LEN - 1) {
                current_token.lexeme[i++] = source[pos];
            }
            pos++;
        }
        current_token.lexeme[i] = '\0';

        // 转为小写检查关键字（先检查关键字再检查长度）
        char lower[MAX_TOKEN_LEN];
        int j;
        for (j = 0; current_token.lexeme[j]; j++) {
            lower[j] = tolower((unsigned char)current_token.lexeme[j]);
        }
        lower[j] = '\0';

        TokenType kw_type;
        if (is_keyword(lower, &kw_type)) {
            current_token.type = kw_type;
            strcpy(current_token.lexeme, lower);
        } else {
            current_token.type = TOK_IDENT;
            // 非关键字的标识符才检查长度
            if (i > MAX_IDENT_LEN) {
                current_token.type = TOK_ERROR;
                current_token.error_type = ERR_LEX_IDENT_TOO_LONG;
                has_lex_error = 1;
            }
        }
        if (current_token.type != TOK_ERROR) {
            token_count_by_type[current_token.type]++;
        }
        return;
    }

    // 数字
    if (isdigit((unsigned char)ch)) {
        int i = 0;
        int value = 0;
        while (isdigit((unsigned char)source[pos])) {
            value = value * 10 + (source[pos] - '0');
            if (i < MAX_TOKEN_LEN - 1) {
                current_token.lexeme[i++] = source[pos];
            }
            pos++;
        }
        current_token.lexeme[i] = '\0';
        current_token.type = TOK_NUMBER;
        current_token.value = value;

        // 检查是否以数字开头但后面跟了字母（非法：如 2a）
        if (isalpha((unsigned char)source[pos]) || source[pos] == '_') {
            // 读取剩余的非法标识符部分
            while (isalnum((unsigned char)source[pos]) || source[pos] == '_') {
                if (i < MAX_TOKEN_LEN - 1) {
                    current_token.lexeme[i++] = source[pos];
                }
                pos++;
            }
            current_token.lexeme[i] = '\0';
            has_lex_error = 1;
            current_token.type = TOK_ERROR;
            current_token.error_type = ERR_LEX_INVALID_IDENT;
            return;
        }

        // 检查数字长度
        if (i > MAX_NUM_LEN) {
            current_token.type = TOK_ERROR;
            current_token.error_type = ERR_LEX_NUM_TOO_LONG;
            has_lex_error = 1;
        }

        if (current_token.type != TOK_ERROR) {
            token_count_by_type[TOK_NUMBER]++;
        }
        return;
    }

    // 运算符和界符
    switch (ch) {
        case '+':
            current_token.type = TOK_PLUS;
            current_token.lexeme[0] = '+'; current_token.lexeme[1] = '\0';
            pos++;
            break;
        case '-':
            current_token.type = TOK_MINUS;
            current_token.lexeme[0] = '-'; current_token.lexeme[1] = '\0';
            pos++;
            break;
        case '*':
            current_token.type = TOK_MUL;
            current_token.lexeme[0] = '*'; current_token.lexeme[1] = '\0';
            pos++;
            break;
        case '/':
            current_token.type = TOK_DIV;
            current_token.lexeme[0] = '/'; current_token.lexeme[1] = '\0';
            pos++;
            break;
        case '=':
            current_token.type = TOK_EQ;
            current_token.lexeme[0] = '='; current_token.lexeme[1] = '\0';
            pos++;
            break;
        case '#':
            current_token.type = TOK_NEQ;
            current_token.lexeme[0] = '#'; current_token.lexeme[1] = '\0';
            pos++;
            break;
        case '<':
            if (source[pos + 1] == '=') {
                current_token.type = TOK_LE;
                current_token.lexeme[0] = '<'; current_token.lexeme[1] = '=';
                current_token.lexeme[2] = '\0';
                pos += 2;
            } else if (source[pos + 1] == '>') {
                current_token.type = TOK_NEQ;
                current_token.lexeme[0] = '<'; current_token.lexeme[1] = '>';
                current_token.lexeme[2] = '\0';
                pos += 2;
            } else {
                current_token.type = TOK_LT;
                current_token.lexeme[0] = '<'; current_token.lexeme[1] = '\0';
                pos++;
            }
            break;
        case '>':
            if (source[pos + 1] == '=') {
                current_token.type = TOK_GE;
                current_token.lexeme[0] = '>'; current_token.lexeme[1] = '=';
                current_token.lexeme[2] = '\0';
                pos += 2;
            } else {
                current_token.type = TOK_GT;
                current_token.lexeme[0] = '>'; current_token.lexeme[1] = '\0';
                pos++;
            }
            break;
        case ':':
            if (source[pos + 1] == '=') {
                current_token.type = TOK_ASSIGN;
                current_token.lexeme[0] = ':'; current_token.lexeme[1] = '=';
                current_token.lexeme[2] = '\0';
                pos += 2;
            } else {
                current_token.type = TOK_ERROR;
                current_token.error_type = ERR_LEX_ILLEGAL_CHAR;
                current_token.lexeme[0] = ':'; current_token.lexeme[1] = '\0';
                pos++;
                has_lex_error = 1;
            }
            break;
        case ',':
            current_token.type = TOK_COMMA;
            current_token.lexeme[0] = ','; current_token.lexeme[1] = '\0';
            pos++;
            break;
        case ';':
            current_token.type = TOK_SEMI;
            current_token.lexeme[0] = ';'; current_token.lexeme[1] = '\0';
            pos++;
            break;
        case '.':
            current_token.type = TOK_PERIOD;
            current_token.lexeme[0] = '.'; current_token.lexeme[1] = '\0';
            pos++;
            break;
        case '(':
            current_token.type = TOK_LPAREN;
            current_token.lexeme[0] = '('; current_token.lexeme[1] = '\0';
            pos++;
            break;
        case ')':
            current_token.type = TOK_RPAREN;
            current_token.lexeme[0] = ')'; current_token.lexeme[1] = '\0';
            pos++;
            break;
        default:
            current_token.type = TOK_ERROR;
            current_token.error_type = ERR_LEX_ILLEGAL_CHAR;
            current_token.lexeme[0] = ch; current_token.lexeme[1] = '\0';
            pos++;
            has_lex_error = 1;
            break;
    }

    if (current_token.type != TOK_ERROR) {
        token_count_by_type[current_token.type]++;
    }
}

const char* get_token_name(TokenType type) {
    switch (type) {
        case TOK_CONST:     return "const";
        case TOK_VAR:       return "var";
        case TOK_PROCEDURE: return "procedure";
        case TOK_IF:        return "if";
        case TOK_THEN:      return "then";
        case TOK_WHILE:     return "while";
        case TOK_DO:        return "do";
        case TOK_CALL:      return "call";
        case TOK_READ:      return "read";
        case TOK_WRITE:     return "write";
        case TOK_BEGIN:     return "begin";
        case TOK_END:       return "end";
        case TOK_ODD:       return "odd";
        case TOK_IDENT:     return "ident";
        case TOK_NUMBER:    return "number";
        case TOK_PLUS:      return "+";
        case TOK_MINUS:     return "-";
        case TOK_MUL:       return "*";
        case TOK_DIV:       return "/";
        case TOK_ASSIGN:    return ":=";
        case TOK_EQ:        return "=";
        case TOK_NEQ:       return "#";
        case TOK_LT:        return "<";
        case TOK_LE:        return "<=";
        case TOK_GT:        return ">";
        case TOK_GE:        return ">=";
        case TOK_COMMA:     return ",";
        case TOK_SEMI:      return ";";
        case TOK_PERIOD:    return ".";
        case TOK_LPAREN:    return "(";
        case TOK_RPAREN:    return ")";
        case TOK_EOF:       return "$";
        default:            return "unknown";
    }
}

// 按任务要求格式输出token
void print_token_for_lexer(Token *t) {
    switch (t->type) {
        case TOK_CONST: case TOK_VAR: case TOK_PROCEDURE:
        case TOK_IF: case TOK_THEN: case TOK_WHILE: case TOK_DO:
        case TOK_CALL: case TOK_READ: case TOK_WRITE:
        case TOK_BEGIN: case TOK_END: case TOK_ODD:
            printf("(保留字,%s)\n", t->lexeme);
            break;
        case TOK_IDENT:
            printf("(标识符,%s)\n", t->lexeme);
            break;
        case TOK_NUMBER:
            printf("(无符号整数,%d)\n", t->value);
            break;
        case TOK_PLUS: case TOK_MINUS: case TOK_MUL: case TOK_DIV:
        case TOK_ASSIGN:
            printf("(运算符,%s)\n", t->lexeme);
            break;
        case TOK_EQ: case TOK_NEQ: case TOK_LT: case TOK_LE:
        case TOK_GT: case TOK_GE:
            printf("(运算符,%s)\n", t->lexeme);
            break;
        case TOK_COMMA: case TOK_SEMI: case TOK_PERIOD:
        case TOK_LPAREN: case TOK_RPAREN:
            printf("(界符,%s)\n", t->lexeme);
            break;
        case TOK_ERROR:
            switch (t->error_type) {
                case ERR_LEX_ILLEGAL_CHAR:
                case ERR_LEX_INVALID_IDENT:
                    printf("(非法字符(串),%s,行号:%d)\n", t->lexeme, t->line_no);
                    break;
                case ERR_LEX_NUM_TOO_LONG:
                    printf("(无符号整数越界,%s,行号:%d)\n", t->lexeme, t->line_no);
                    break;
                case ERR_LEX_IDENT_TOO_LONG:
                    printf("(标识符长度超长,%s,行号:%d)\n", t->lexeme, t->line_no);
                    break;
                default:
                    break;
            }
            break;
        case TOK_EOF:
            break;
        default:
            break;
    }
}

void get_token_string(Token *t, char *buf) {
    const char *name = get_token_name(t->type);
    if (t->type == TOK_IDENT || t->type == TOK_NUMBER) {
        sprintf(buf, "%s(%s)", name, t->lexeme);
    } else {
        strcpy(buf, name);
    }
}

// 一次性词法分析，将所有Token存入列表（末尾追加EOF哨兵）
void tokenize_all(const char *source, TokenList *tl) {
    init_lexer(source);
    tl->count = 0;
    tl->pos = 0;

    next_token();
    while (current_token.type != TOK_EOF) {
        if (tl->count < MAX_TOKEN_LIST - 1) {  // 预留EOF哨兵位置
            tl->tokens[tl->count] = current_token;
            tl->count++;
        }
        next_token();
    }
    // 追加EOF哨兵
    tl->tokens[tl->count].type = TOK_EOF;
    tl->tokens[tl->count].line_no = current_token.line_no;
    strcpy(tl->tokens[tl->count].lexeme, "$");
}
