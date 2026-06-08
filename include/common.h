#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 32
#define MAX_IDENT_LEN 8
#define MAX_NUM_LEN 8
#define MAX_LINE_LEN 256
#define MAX_SOURCE_SIZE 65536
#define MAX_TOKEN_LIST 4096
#define MAX_SYMBOL_TABLE 200
#define MAX_QUAD_SIZE 500
#define MAX_STACK_SIZE 200

// ============== Token类型枚举 ==============
typedef enum {
    // 保留字
    TOK_CONST, TOK_VAR, TOK_PROCEDURE,
    TOK_IF, TOK_THEN, TOK_WHILE, TOK_DO,
    TOK_CALL, TOK_READ, TOK_WRITE,
    TOK_BEGIN, TOK_END, TOK_ODD,
    
    // 标识符和数字
    TOK_IDENT, TOK_NUMBER,
    
    // 运算符
    TOK_PLUS, TOK_MINUS, TOK_MUL, TOK_DIV,
    TOK_ASSIGN, TOK_EQ, TOK_NEQ,
    TOK_LT, TOK_LE, TOK_GT, TOK_GE,
    
    // 界符
    TOK_COMMA, TOK_SEMI, TOK_PERIOD,
    TOK_LPAREN, TOK_RPAREN,
    
    // 特殊
    TOK_EOF, TOK_ERROR
} TokenType;

// Token结构
typedef struct {
    TokenType type;
    char lexeme[MAX_TOKEN_LEN];
    int line_no;
    int value;
} Token;

// Token列表 — 词法分析输出，供语法/语义分析消费
typedef struct {
    Token tokens[MAX_TOKEN_LIST];
    int count;   // Token总数
    int pos;     // 当前读取位置
} TokenList;

// 符号表项类型
typedef enum {
    SYM_CONST, SYM_VAR, SYM_PROC
} SymType;

// 符号表项
typedef struct {
    char name[MAX_IDENT_LEN + 1];
    SymType type;
    int value;      // 常量值或变量地址
    int level;      // 作用域层数
    int addr;       // 地址
} Symbol;

// 四元式结构
typedef struct {
    char op[16];    // 操作符
    char arg1[16];  // 第一操作数
    char arg2[16];  // 第二操作数
    char result[16]; // 结果
} Quad;

// 错误类型
typedef enum {
    ERR_LEX_ILLEGAL_CHAR,
    ERR_LEX_INVALID_IDENT,
    ERR_LEX_NUM_TOO_LONG,
    ERR_LEX_IDENT_TOO_LONG,
    ERR_SYNTAX,
    ERR_SEM_DUPLICATE,
    ERR_SEM_UNDECLARED,
    ERR_SEM_TYPE_MISMATCH
} ErrorType;

#endif