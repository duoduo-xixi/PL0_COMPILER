#ifndef LR_PARSER_H
#define LR_PARSER_H

#include "../include/common.h"

// 语法分析结果
typedef struct {
    int success;
    char error_msg[256];
    int error_line;
} ParseResult;

// 执行LR语法分析（消费TokenList中的Token）
ParseResult lr_parse(TokenList *token_list);

#endif