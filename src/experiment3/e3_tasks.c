#include "e3_tasks.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// ==================== 任务1-1：密文字符频率分析 ====================
void e3_task1_freq(const char *input) {
    int freq[26] = {0};
    int total = 0;

    for (const char *p = input; *p; p++) {
        if (isalpha((unsigned char)*p)) {
            char c = toupper((unsigned char)*p);
            freq[c - 'A']++;
            total++;
        }
        // 忽略空格、数字、标点等其他字符
    }

    if (total == 0) {
        printf("No letters found.\n");
        return;
    }

    int first = 1;
    for (int i = 0; i < 26; i++) {
        if (freq[i] > 0) {
            if (!first) printf(", ");
            double percent = (freq[i] * 100.0) / total;
            printf("%c: %d (%.2f%%)", 'A' + i, freq[i], percent);
            first = 0;
        }
    }
    printf("\n");
}

// ==================== 任务1-2：识别单词、数字和符号 ====================
static int is_ident_start(int c) {
    return isalpha((unsigned char)c) || c == '_';
}

static int is_ident_part(int c) {
    return isalnum((unsigned char)c) || c == '_';
}

void e3_task2_tokenizer(const char *input) {
    const char *p = input;

    while (*p) {
        // 跳过空格和制表符
        if (*p == ' ' || *p == '\t') {
            p++;
            continue;
        }
        // 跳过换行
        if (*p == '\n' || *p == '\r') {
            p++;
            continue;
        }

        // 单词：字母或下划线开头，后跟字母/数字/下划线
        if (is_ident_start(*p)) {
            char buf[256];
            int i = 0;
            while (is_ident_part(*p) && i < 255) {
                buf[i++] = *p++;
            }
            buf[i] = '\0';
            printf("%s 单词\n", buf);
            continue;
        }

        // 数字
        if (isdigit((unsigned char)*p)) {
            char buf[256];
            int i = 0;
            while (isdigit((unsigned char)*p) && i < 255) {
                buf[i++] = *p++;
            }
            buf[i] = '\0';
            printf("%s 数字\n", buf);
            continue;
        }

        // 字符串字面量（支持 "..." 和 << 中包含的 "..." ）
        if (*p == '"') {
            char buf[256];
            int i = 0;
            buf[i++] = *p++;
            while (*p && *p != '"' && i < 254) {
                buf[i++] = *p++;
            }
            if (*p == '"' && i < 255) {
                buf[i++] = *p++;
            }
            buf[i] = '\0';
            printf("%s 符号\n", buf);
            continue;
        }

        // 双字符符号 <<, >>, <=, >=, ==, !=, :=
        if ((*p == '<' && *(p+1) == '<') ||
            (*p == '>' && *(p+1) == '>') ||
            (*p == '<' && *(p+1) == '=') ||
            (*p == '>' && *(p+1) == '=') ||
            (*p == '=' && *(p+1) == '=') ||
            (*p == '!' && *(p+1) == '=') ||
            (*p == ':' && *(p+1) == '='))
        {
            printf("%c%c 符号\n", *p, *(p+1));
            p += 2;
            continue;
        }

        // 单字符符号：其他非空白非字母数字下划线
        {
            printf("%c 符号\n", *p);
            p++;
        }
    }
}

// ==================== 任务1-3：计算器（支持 + 和 *）====================
// 简单递归下降：表达式 = 项 { '+' 项 }，项 = 因子 { '*' 因子 }，因子 = 数字
static const char *calc_p;
static int calc_has_error;

static int parse_number(void) {
    while (*calc_p == ' ' || *calc_p == '\t') calc_p++;
    if (!isdigit((unsigned char)*calc_p)) {
        calc_has_error = 1;
        return 0;
    }
    int val = 0;
    while (isdigit((unsigned char)*calc_p)) {
        val = val * 10 + (*calc_p - '0');
        calc_p++;
    }
    return val;
}

static int parse_factor(void) {
    while (*calc_p == ' ' || *calc_p == '\t') calc_p++;
    return parse_number();
}

static int parse_term(void) {
    int val = parse_factor();
    while (*calc_p) {
        while (*calc_p == ' ' || *calc_p == '\t') calc_p++;
        if (*calc_p == '*') {
            calc_p++;
            val *= parse_factor();
        } else {
            break;
        }
    }
    return val;
}

static int parse_expr(void) {
    int val = parse_term();
    while (*calc_p) {
        while (*calc_p == ' ' || *calc_p == '\t') calc_p++;
        if (*calc_p == '+') {
            calc_p++;
            val += parse_term();
        } else {
            break;
        }
    }
    return val;
}

void e3_task3_calc(const char *input) {
    calc_p = input;
    calc_has_error = 0;

    while (*calc_p) {
        // 跳过空白和换行
        while (*calc_p == ' ' || *calc_p == '\t' || *calc_p == '\n' || *calc_p == '\r')
            calc_p++;
        if (!*calc_p) break;

        // 检查是否是空行
        if (*calc_p == '\0') break;

        calc_has_error = 0;
        int result = parse_expr();

        if (!calc_has_error) {
            // 跳过表达式后的空白
            while (*calc_p == ' ' || *calc_p == '\t') calc_p++;
            // 期望换行或EOF
            if (*calc_p == '\n' || *calc_p == '\r') {
                // 消费换行
                while (*calc_p == '\n' || *calc_p == '\r') calc_p++;
            } else if (*calc_p != '\0') {
                // 还有更多字符但不是换行，忽略行内剩余字符
                calc_has_error = 1;
            }
        }

        if (calc_has_error) {
            printf("Error\n");
            // 跳到下一行
            while (*calc_p && *calc_p != '\n') calc_p++;
            if (*calc_p == '\n') calc_p++;
        } else {
            printf("%d\n", result);
        }
    }
}
