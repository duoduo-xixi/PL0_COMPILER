#ifndef LEXER_H
#define LEXER_H

#include "../include/common.h"

extern int current_line;
extern int has_lex_error;
extern Token current_token;

// 初始化词法分析器
void init_lexer(const char *source);

// 获取下一个Token
void next_token(void);

// 一次性词法分析：分析整个源文件，将所有Token存入列表
void tokenize_all(const char *source, TokenList *tl);

// 获取Token名称
const char* get_token_name(TokenType type);

// 获取Token的字符串表示
void get_token_string(Token *t, char *buf);

// 按任务要求格式输出Token
void print_token_for_lexer(Token *t);

// 输出词法统计
void print_token_stats(void);

#endif