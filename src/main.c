#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include "../include/common.h"
#include "lexer.h"
#include "lr_parser.h"
#include "semantic.h"
#include "experiment3/e3_tasks.h"
// ==================== 文件读取 ====================
char* read_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return NULL;

    char *buffer = (char*)malloc(MAX_SOURCE_SIZE);
    if (!buffer) { fclose(fp); return NULL; }

    size_t len = fread(buffer, 1, MAX_SOURCE_SIZE - 1, fp);
    buffer[len] = '\0';
    fclose(fp);
    return buffer;
}

// ==================== 运行模式 ====================
typedef enum {
    MODE_LEXER_ONLY,       // 仅词法分析
    MODE_LEXER_PARSER,     // 词法 + 语法
    MODE_FULL              // 词法 + 语法 + 语义
} RunMode;

// 仅运行词法分析（实验四）
void run_lexer_only(const char *source_code) {
    TokenList tl;
    tokenize_all(source_code, &tl);

    printf("\n========== 词法分析结果 ==========\n");
    for (int i = 0; i < tl.count; i++) {
        print_token_for_lexer(&tl.tokens[i]);
    }

    if (has_lex_error) {
        printf("\n词法分析发现错误。\n");
    }
}

// 运行词法 + 语法分析（实验五）
void run_lexer_and_parser(const char *source_code) {
    TokenList tl;

    // 第一步：词法分析，产生TokenList
    tokenize_all(source_code, &tl);

    printf("\n========== 词法分析结果 ==========\n");
    for (int i = 0; i < tl.count; i++) {
        print_token_for_lexer(&tl.tokens[i]);
    }

    if (has_lex_error) {
        printf("词法分析发现错误，停止后续分析。\n");
        return;
    }

    // 第二步：语法分析，消费TokenList
    ParseResult parse_result = lr_parse(&tl);

    if (!parse_result.success) {
        printf("语法分析发现错误。\n");
        return;
    }
}

// 运行完整编译流程：词法 → 语法 → 语义（实验六）
void run_full_compiler(const char *source_code) {
    TokenList tl;

    // 第一步：词法分析，产生TokenList
    tokenize_all(source_code, &tl);

    printf("\n========== 词法分析结果 ==========\n");
    for (int i = 0; i < tl.count; i++) {
        print_token_for_lexer(&tl.tokens[i]);
    }

    if (has_lex_error) {
        printf("词法分析发现错误，停止后续分析。\n");
        return;
    }

    // 第二步：语法分析，消费TokenList
    ParseResult parse_result = lr_parse(&tl);

    if (!parse_result.success) {
        printf("语法分析发现错误。\n");
        return;
    }

    // 第三步：语义分析，重新消费TokenList
    printf("\n========== 语义分析 ==========\n");
    semantic_analyze(&tl);
}

// ==================== 测试用例定义 ====================
typedef struct {
    const char *filename;
    const char *description;
} TestCase;

typedef struct {
    const char *name;
    const char *dir;
    const TestCase *tests;
    int count;
    RunMode mode;
} Experiment;

// ---- 实验四：词法分析 ----
static const TestCase e4_tests[] = {
    {"e4_t1_correct.pl0", "正确词法测试"},
    {"e4_t2_error.pl0",   "错误词法测试（非法字符/数字越界/标识符过长）"},
};

// ---- 实验五：语法分析 ----
static const TestCase e5_tests[] = {
    {"e5_t1_correct.pl0",      "正确语法测试"},
    {"e5_t2_assign_error.pl0", "const a := 10（:= 代替 =）"},
    {"e5_t3_no_begin.pl0",     "缺少程序头与 begin"},
    {"e5_t4_no_do.pl0",        "while 缺少 do"},
    {"e5_t5_multi_error.pl0",  "多个语法错误（8个错误）"},
};

// ---- 实验六：语义分析 ----
static const TestCase e6_tests[] = {
    {"e6_t1_correct.pl0",       "正确语义测试（含注释）"},
    {"e6_t2_dup_declare.pl0",   "重复声明 a"},
    {"e6_t3_read_proc.pl0",     "read(p)——p 是过程不是变量"},
    {"e6_t4_undefined_proc.pl0","call q——q 未声明"},
    {"e6_t5_multi_semantic.pl0","多个语义错误（4个错误）"},
};

static const Experiment experiments[] = {
    {"实验四：词法分析", "tests/experiment4_lexer",   e4_tests, 2, MODE_LEXER_ONLY},
    {"实验五：语法分析", "tests/experiment5_parser",  e5_tests, 5, MODE_LEXER_PARSER},
    {"实验六：语义分析", "tests/experiment6_semantic", e6_tests, 5, MODE_FULL},
};

#define NUM_EXPERIMENTS (sizeof(experiments) / sizeof(experiments[0]))

// ==================== 运行单个测试文件 ====================
void run_test_file(const char *filepath, RunMode mode, const char *output_path) {
    char *source = read_file(filepath);
    if (!source) {
        printf("无法打开文件: %s\n", filepath);
        return;
    }

    printf("\n----------------------------------------\n");
    printf("源代码:\n%s\n", source);
    printf("----------------------------------------\n");

    int saved_fd = -1;
    if (output_path) {
        saved_fd = _dup(1);
        if (saved_fd != -1) {
            if (!freopen(output_path, "w", stdout)) {
                _close(saved_fd);
                saved_fd = -1;
            }
        }
    }

    switch (mode) {
        case MODE_LEXER_ONLY:
            run_lexer_only(source);
            break;
        case MODE_LEXER_PARSER:
            run_lexer_and_parser(source);
            break;
        case MODE_FULL:
            run_full_compiler(source);
            break;
    }

    if (saved_fd != -1) {
        fflush(stdout);
        _dup2(saved_fd, 1);
        _close(saved_fd);
        clearerr(stdout);
        setvbuf(stdout, NULL, _IONBF, 0);

        // 回显文件内容到控制台
        char *result = read_file(output_path);
        if (result) {
            printf("%s", result);
            free(result);
        }
        printf("[结果已保存到: %s]\n", output_path);
    }

    free(source);
}

// ==================== 运行一个实验的全部测试 ====================
void run_experiment(const Experiment *exp) {
    printf("\n========================================\n");
    printf("  %s\n", exp->name);
    printf("========================================\n");

    // 创建输出目录: output/<实验目录名>
    char out_dir[512];
    const char *dir_name = strrchr(exp->dir, '/');
    dir_name = dir_name ? dir_name + 1 : exp->dir;
    _mkdir("output");
    snprintf(out_dir, sizeof(out_dir), "output/%s", dir_name);
    _mkdir(out_dir);

    for (int i = 0; i < exp->count; i++) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", exp->dir, exp->tests[i].filename);

        printf("\n--------------------------------------------------\n");
        printf("[%d/%d] %s: %s\n", i + 1, exp->count,
               exp->tests[i].filename, exp->tests[i].description);
        printf("--------------------------------------------------\n");

        // 构造输出路径: output/<实验目录>/<测试名>.txt
        char out_path[512];
        const char *dot = strrchr(exp->tests[i].filename, '.');
        int base_len = dot ? (int)(dot - exp->tests[i].filename)
                           : (int)strlen(exp->tests[i].filename);
        snprintf(out_path, sizeof(out_path), "%s/%.*s.txt",
                 out_dir, base_len, exp->tests[i].filename);

        run_test_file(filepath, exp->mode, out_path);
    }
}

// ==================== 实验三：Lex和yacc的使用 ====================
static const TestCase e3_tests[] = {
    {"e3_t1_freq.txt",       "任务1-1：密文字符频率分析"},
    {"e3_t2_tokenizer.txt",  "任务1-2：识别单词、数字和符号"},
    {"e3_t3_calc.txt",       "任务1-3：具有加法和乘法功能的计算器"},
};

void run_experiment3(void) {
    const char *dir = "tests/experiment3_lex_yacc";

    printf("\n========================================\n");
    printf("  实验三：Lex和yacc的使用\n");
    printf("========================================\n");

    // 创建输出目录
    _mkdir("output");
    _mkdir("output/experiment3_lex_yacc");

    for (int i = 0; i < 3; i++) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", dir, e3_tests[i].filename);

        printf("\n--------------------------------------------------\n");
        printf("[%d/3] %s: %s\n", i + 1, e3_tests[i].filename, e3_tests[i].description);
        printf("--------------------------------------------------\n");

        // 读取输入文件
        char *input = read_file(filepath);
        if (!input) {
            printf("无法打开输入文件: %s\n", filepath);
            continue;
        }

        printf("输入:\n%s\n", input);
        printf("输出:\n");

        // 构造输出路径
        char out_path[512];
        snprintf(out_path, sizeof(out_path), "output/experiment3_lex_yacc/%s",
                 e3_tests[i].filename);

        // 重定向stdout到文件
        int saved_fd = _dup(1);
        if (saved_fd != -1) {
            if (!freopen(out_path, "w", stdout)) {
                _close(saved_fd);
                saved_fd = -1;
            }
        }

        // 执行对应任务
        switch (i) {
            case 0: e3_task1_freq(input);       break;
            case 1: e3_task2_tokenizer(input);  break;
            case 2: e3_task3_calc(input);       break;
        }

        // 恢复stdout
        if (saved_fd != -1) {
            fflush(stdout);
            _dup2(saved_fd, 1);
            _close(saved_fd);
            clearerr(stdout);
            setvbuf(stdout, NULL, _IONBF, 0);

            // 回显文件内容
            char *result = read_file(out_path);
            if (result) {
                printf("%s", result);
                free(result);
            }
            printf("[结果已保存到: %s]\n", out_path);
        }

        free(input);
    }
}

// ==================== 运行全部实验 ====================
void run_all_experiments(void) {
    for (int i = 0; i < NUM_EXPERIMENTS; i++) {
        run_experiment(&experiments[i]);
    }
}

// ==================== 自定义测试 ====================
void run_custom_test(void) {
    printf("\n请输入 .pl0 文件的路径: ");
    char filepath[512];
    if (!fgets(filepath, sizeof(filepath), stdin)) return;

    // 去掉末尾换行符
    size_t len = strlen(filepath);
    while (len > 0 && (filepath[len - 1] == '\n' || filepath[len - 1] == '\r')) {
        filepath[--len] = '\0';
    }

    if (len == 0) {
        printf("未输入文件路径。\n");
        return;
    }

    printf("\n请选择运行模式:\n");
    printf("  1. 仅词法分析\n");
    printf("  2. 词法 + 语法分析\n");
    printf("  3. 完整编译（词法 + 语法 + 语义）\n");
    printf("请输入选择 (1-3): ");

    int choice;
    if (scanf("%d", &choice) != 1) {
        printf("无效选择。\n");
        getchar();
        return;
    }
    getchar();

    RunMode mode;
    switch (choice) {
        case 1:  mode = MODE_LEXER_ONLY;    break;
        case 2:  mode = MODE_LEXER_PARSER;  break;
        case 3:  mode = MODE_FULL;          break;
        default: printf("无效选择。\n");     return;
    }

    // 构造输出路径: output/custom/<文件名>.txt
    char out_path[512];
    {
        const char *filename = strrchr(filepath, '/');
        if (!filename) filename = strrchr(filepath, '\\');
        filename = filename ? filename + 1 : filepath;
        const char *dot = strrchr(filename, '.');
        int base_len = dot ? (int)(dot - filename) : (int)strlen(filename);
        _mkdir("output");
        _mkdir("output/custom");
        snprintf(out_path, sizeof(out_path), "output/custom/%.*s.txt",
                 base_len, filename);
    }

    run_test_file(filepath, mode, out_path);
}

// ==================== 主菜单 ====================
void show_menu(void) {
    printf("\n");
    printf("========================================\n");
    printf("   PL/0 编译器 - 词法/语法/语义分析\n");
    printf("========================================\n");
    printf("\n选择测试用例 (按实验顺序):\n");
    printf("  [0] 实验三：Lex和yacc的使用  (%d 个测试)\n", 3);
    printf("  [1] 实验四：词法分析  (%d 个测试)\n", experiments[0].count);
    printf("  [2] 实验五：语法分析  (%d 个测试)\n", experiments[1].count);
    printf("  [3] 实验六：语义分析  (%d 个测试)\n", experiments[2].count);
    printf("  [A] 运行全部测试（按实验四→五→六顺序）\n");
    printf("  [C] 自定义测试（输入文件路径）\n");
    printf("  [Q] 退出\n");
    printf("请输入选择: ");
}

// ==================== 主函数 ====================
int main(int argc, char *argv[]) {
    system("chcp 65001 > nul");

    if (argc >= 2) {
        // 命令行模式：直接编译指定文件
        {
            // 构造输出路径: output/custom/<文件名>.txt
            const char *fn = strrchr(argv[1], '/');
            if (!fn) fn = strrchr(argv[1], '\\');
            fn = fn ? fn + 1 : argv[1];
            const char *dot = strrchr(fn, '.');
            int base_len = dot ? (int)(dot - fn) : (int)strlen(fn);
            char out_path[512];
            _mkdir("output");
            _mkdir("output/custom");
            snprintf(out_path, sizeof(out_path), "output/custom/%.*s.txt",
                     base_len, fn);
            run_test_file(argv[1], MODE_FULL, out_path);
        }
        return 0;
    }

    // 交互式菜单
    while (1) {
        show_menu();

        char line[32];
        if (!fgets(line, sizeof(line), stdin)) break;

        // 去掉换行符
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        if (len == 0) continue;

        char choice = line[0];

        switch (choice) {
            case '0': run_experiment3();                  break;
            case '1': run_experiment(&experiments[0]); break;
            case '2': run_experiment(&experiments[1]); break;
            case '3': run_experiment(&experiments[2]); break;
            case 'A': case 'a': run_all_experiments(); break;
            case 'C': case 'c': run_custom_test();     break;
            case 'Q': case 'q':
                printf("退出。\n");
                return 0;
            default:
                printf("无效选择，请重新输入。\n");
                break;
        }
    }

    return 0;
}
