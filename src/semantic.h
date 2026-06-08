#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "../include/common.h"

// 符号表操作
void init_symbol_table(void);
int add_symbol(const char *name, SymType type, int value);
Symbol* find_symbol(const char *name);

// 四元式操作
void init_quads(void);
int add_quad(const char *op, const char *arg1, const char *arg2, const char *result);
void print_quads(void);

// 语义分析入口（消费TokenList中的Token）
void semantic_analyze(TokenList *token_list);

// 错误处理
void semantic_error(ErrorType err, int line, const char *detail);

extern int semantic_has_error;
extern int quad_count;

#endif