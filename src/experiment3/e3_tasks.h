#ifndef E3_TASKS_H
#define E3_TASKS_H

// 实验三：Lex和yacc的使用 — 三个子任务
// 原始实现使用Flex/Bison工具，此处用C语言复现等价功能

// 任务1-1：密文字符频率分析（凯撒密码字符频率统计）
// 只统计大写字母，小写字母转为大写统计，忽略空格、标点及数字
void e3_task1_freq(const char *input);

// 任务1-2：识别单词、数字和符号（空格不作处理）
void e3_task2_tokenizer(const char *input);

// 任务1-3：具有加法和乘法功能的计算器
void e3_task3_calc(const char *input);

#endif
