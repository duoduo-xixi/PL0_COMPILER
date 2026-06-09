#include "semantic.h"
#include <stdio.h>
#include <string.h>

// 符号表
static Symbol symbol_table[MAX_SYMBOL_TABLE];
static int symbol_count = 0;

// 四元式表
static Quad quads[MAX_QUAD_SIZE];
int quad_count = 0;

int semantic_has_error = 0;
static int temp_counter = 1;  // 临时变量计数器

// 错误处理
void semantic_error(ErrorType err, int line, const char *detail) {
    semantic_has_error = 1;
    printf("(语义错误,行号:%d)\n", line);
}

// 初始化符号表
void init_symbol_table(void) {
    symbol_count = 0;
    temp_counter = 1;
    memset(symbol_table, 0, sizeof(symbol_table));
}

// 添加符号
int add_symbol(const char *name, SymType type, int value) {
    int i;
    for (i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return 0;  // 重复
        }
    }
    if (symbol_count >= MAX_SYMBOL_TABLE) return 0;
    strncpy(symbol_table[symbol_count].name, name, MAX_IDENT_LEN);
    symbol_table[symbol_count].type = type;
    symbol_table[symbol_count].value = value;
    symbol_table[symbol_count].level = 0;
    symbol_table[symbol_count].addr = symbol_count;
    symbol_count++;
    return 1;
}

// 查找符号
Symbol* find_symbol(const char *name) {
    int i;
    for (i = symbol_count - 1; i >= 0; i--) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return &symbol_table[i];
        }
    }
    return NULL;
}

// 初始化四元式
void init_quads(void) {
    quad_count = 0;
    temp_counter = 1;
    memset(quads, 0, sizeof(quads));
}

// 添加四元式
int add_quad(const char *op, const char *arg1, const char *arg2, const char *result) {
    if (quad_count >= MAX_QUAD_SIZE) return -1;
    strcpy(quads[quad_count].op, op);
    strcpy(quads[quad_count].arg1, arg1);
    strcpy(quads[quad_count].arg2, arg2);
    strcpy(quads[quad_count].result, result);
    return quad_count++;
}

// 生成临时变量名
static char* new_temp(void) {
    static char temp_name[16];
    sprintf(temp_name, "T%d", temp_counter++);
    return temp_name;
}

// 打印符号表
void print_symbol_table(void) {
    int i;
    printf("\n========== 符号表 ==========\n");
    for (i = 0; i < symbol_count; i++) {
        if (symbol_table[i].type == SYM_CONST) {
            printf("const %s = %d\n", symbol_table[i].name, symbol_table[i].value);
        } else if (symbol_table[i].type == SYM_VAR) {
            printf("var %s\n", symbol_table[i].name);
        } else if (symbol_table[i].type == SYM_PROC) {
            printf("procedure %s\n", symbol_table[i].name);
        }
    }
}

// 打印四元式
void print_quads(void) {
    int i;
    printf("\n========== 中间代码(四元式) ==========\n");
    for (i = 0; i < quad_count; i++) {
        printf("(%d)(%s,%s,%s,%s)\n", i+1, quads[i].op, 
               quads[i].arg1, quads[i].arg2, quads[i].result);
    }
}

// ============== 语法驱动的语义分析 ==============

// 前向声明
static void parse_program(void);
static void parse_block(void);
static void parse_const_decl(void);
static void parse_var_decl(void);
static void parse_proc_decl(void);
static void parse_statement(void);
static char* parse_expr(void);
static char* parse_term(void);
static char* parse_factor(void);
static void parse_condition(char *jump_op, char *label);

static TokenList *tl;
#define LA (tl->tokens[tl->pos])

// 匹配当前Token并前进
static void match(TokenType expected) {
    if (LA.type == expected) {
        if (LA.type != TOK_EOF) tl->pos++;
    } else {
        semantic_error(ERR_SYNTAX, LA.line_no, "语法错误");
    }
}

// 检查标识符是否已声明
static void check_declared(const char *name, int line) {
    if (find_symbol(name) == NULL) {
        semantic_error(ERR_SEM_UNDECLARED, line, name);
    }
}

// 检查标识符是否为变量（用于 read 语句 & 赋值左值）
static void check_is_variable(const char *name, int line) {
    Symbol *sym = find_symbol(name);
    if (sym == NULL) {
        semantic_error(ERR_SEM_UNDECLARED, line, name);
        return;
    }
    if (sym->type != SYM_VAR) {
        semantic_error(ERR_SEM_TYPE_MISMATCH, line, name);
    }
}

// 检查标识符是否为过程（用于 call 语句）
static void check_is_procedure(const char *name, int line) {
    Symbol *sym = find_symbol(name);
    if (sym == NULL) {
        semantic_error(ERR_SEM_UNDECLARED, line, name);
        return;
    }
    if (sym->type != SYM_PROC) {
        semantic_error(ERR_SEM_TYPE_MISMATCH, line, name);
    }
}

// 解析程序
static void parse_program(void) {
    add_quad("syss", "_", "_", "_");
    parse_block();
    match(TOK_PERIOD);
    add_quad("syse", "_", "_", "_");
}

// 解析分程序
static void parse_block(void) {
    parse_const_decl();
    parse_var_decl();
    parse_proc_decl();
    parse_statement();
}

// 解析常量说明部分
static void parse_const_decl(void) {
    if (LA.type == TOK_CONST) {
        match(TOK_CONST);
        do {
            char ident[MAX_IDENT_LEN+1];
            int value;
            
            if (LA.type != TOK_IDENT) {
                semantic_error(ERR_SYNTAX, LA.line_no, "缺少标识符");
                return;
            }
            strcpy(ident, LA.lexeme);
            match(TOK_IDENT);
            match(TOK_EQ);
            if (LA.type != TOK_NUMBER) {
                semantic_error(ERR_SYNTAX, LA.line_no, "缺少数字");
                return;
            }
            value = LA.value;
            match(TOK_NUMBER);
            
            // 添加常量到符号表
            if (!add_symbol(ident, SYM_CONST, value)) {
                semantic_error(ERR_SEM_DUPLICATE, LA.line_no, ident);
            }
            add_quad("const", ident, "_", "_");
            char val_buf[16];
            sprintf(val_buf, "%d", value);
            add_quad("=", val_buf, "_", ident);
            
        } while (LA.type == TOK_COMMA && (match(TOK_COMMA), 1));
        match(TOK_SEMI);
    }
}

// 解析变量说明部分
static void parse_var_decl(void) {
    if (LA.type == TOK_VAR) {
        match(TOK_VAR);
        do {
            char ident[MAX_IDENT_LEN+1];
            if (LA.type != TOK_IDENT) {
                semantic_error(ERR_SYNTAX, LA.line_no, "缺少标识符");
                return;
            }
            strcpy(ident, LA.lexeme);
            match(TOK_IDENT);
            
            if (!add_symbol(ident, SYM_VAR, 0)) {
                semantic_error(ERR_SEM_DUPLICATE, LA.line_no, ident);
            }
            add_quad("var", ident, "_", "_");
            
        } while (LA.type == TOK_COMMA && (match(TOK_COMMA), 1));
        match(TOK_SEMI);
    }
}

// 解析过程说明部分
static void parse_proc_decl(void) {
    while (LA.type == TOK_PROCEDURE) {
        match(TOK_PROCEDURE);
        char ident[MAX_IDENT_LEN+1];
        if (LA.type != TOK_IDENT) {
            semantic_error(ERR_SYNTAX, LA.line_no, "缺少过程名");
            return;
        }
        strcpy(ident, LA.lexeme);
        match(TOK_IDENT);
        match(TOK_SEMI);
        
        if (!add_symbol(ident, SYM_PROC, 0)) {
            semantic_error(ERR_SEM_DUPLICATE, LA.line_no, ident);
        }
        add_quad("procedure", ident, "_", "_");
        
        parse_block();
        match(TOK_SEMI);
        add_quad("ret", "_", "_", "_");
    }
}

// 解析语句
static void parse_statement(void) {
    if (LA.type == TOK_IDENT) {
        // 赋值语句
        char ident[MAX_IDENT_LEN+1];
        strcpy(ident, LA.lexeme);
        check_is_variable(ident, LA.line_no);
        match(TOK_IDENT);
        match(TOK_ASSIGN);
        char *expr_result = parse_expr();
        add_quad(":=", expr_result, "_", ident);
        
    } else if (LA.type == TOK_IF) {
        // 条件语句
        match(TOK_IF);
        
        char jmp_label[16];
        static int label_counter = 1;
        sprintf(jmp_label, "$L%d", label_counter++);
        
        // 解析条件并生成条件跳转
        char relop[8];
        strcpy(relop, "j");
        parse_condition(relop, jmp_label);
        
        match(TOK_THEN);
        parse_statement();

        add_quad(jmp_label, "_", "_", "_");
        
    } else if (LA.type == TOK_WHILE) {
        // 循环语句
        match(TOK_WHILE);
        
        static int while_counter = 1;
        char start_label[16], end_label[16];
        sprintf(start_label, "$W%d", while_counter);
        sprintf(end_label, "$WE%d", while_counter);
        while_counter++;
        
        add_quad(start_label, "_", "_", "_");
        
        char relop[8];
        strcpy(relop, "j");
        parse_condition(relop, end_label);
        
        match(TOK_DO);
        parse_statement();
        
        add_quad("j", "_", "_", start_label);
        add_quad(end_label, "_", "_", "_");
        
    } else if (LA.type == TOK_CALL) {
        // 过程调用语句
        match(TOK_CALL);
        if (LA.type != TOK_IDENT) {
            semantic_error(ERR_SYNTAX, LA.line_no, "缺少过程名");
            return;
        }
        check_is_procedure(LA.lexeme, LA.line_no);
        add_quad("call", LA.lexeme, "_", "_");
        match(TOK_IDENT);
        
    } else if (LA.type == TOK_READ) {
        // 读语句
        match(TOK_READ);
        match(TOK_LPAREN);
        if (LA.type != TOK_IDENT) {
            semantic_error(ERR_SYNTAX, LA.line_no, "缺少变量名");
            return;
        }
        check_is_variable(LA.lexeme, LA.line_no);
        add_quad("read", LA.lexeme, "_", "_");
        match(TOK_IDENT);
        while (LA.type == TOK_COMMA) {
            match(TOK_COMMA);
            if (LA.type != TOK_IDENT) {
                semantic_error(ERR_SYNTAX, LA.line_no, "缺少变量名");
                return;
            }
            check_is_variable(LA.lexeme, LA.line_no);
            add_quad("read", LA.lexeme, "_", "_");
            match(TOK_IDENT);
        }
        match(TOK_RPAREN);
        
    } else if (LA.type == TOK_WRITE) {
        // 写语句
        match(TOK_WRITE);
        match(TOK_LPAREN);
        char *expr_result = parse_expr();
        add_quad("write", expr_result, "_", "_");
        while (LA.type == TOK_COMMA) {
            match(TOK_COMMA);
            expr_result = parse_expr();
            add_quad("write", expr_result, "_", "_");
        }
        match(TOK_RPAREN);
        
    } else if (LA.type == TOK_BEGIN) {
        // 复合语句
        match(TOK_BEGIN);
        parse_statement();
        while (LA.type == TOK_SEMI) {
            match(TOK_SEMI);
            parse_statement();
        }
        match(TOK_END);
        
    } else {
        // 空语句
    }
}

// 解析条件
static void parse_condition(char *jump_op, char *label) {
    if (LA.type == TOK_ODD) {
        match(TOK_ODD);
        char *expr_result = parse_expr();
        char temp[32];
        strcpy(temp, label);
        add_quad("jodd", expr_result, "_", temp);
    } else {
        char *left = parse_expr();
        char op[8] = {0};
        if (LA.type == TOK_EQ) { op[0] = '='; op[1] = '\0'; } else
        if (LA.type == TOK_NEQ) { op[0] = '#'; op[1] = '\0'; } else
        if (LA.type == TOK_LT) { op[0] = '<'; op[1] = '\0'; } else
        if (LA.type == TOK_LE) strcpy(op, "<="); else
        if (LA.type == TOK_GT) { op[0] = '>'; op[1] = '\0'; } else
        if (LA.type == TOK_GE) strcpy(op, ">="); else {
            semantic_error(ERR_SYNTAX, LA.line_no, "缺少关系运算符");
            return;
        }

        strcat(jump_op, op);
        match(LA.type);
        char *right = parse_expr();

        char temp[32];
        strcpy(temp, label);
        add_quad(jump_op, left, right, temp);
    }
}

// 解析表达式
static char* parse_expr(void) {
    char *term_result = parse_term();
    
    while (LA.type == TOK_PLUS || LA.type == TOK_MINUS) {
        char op[2];
        if (LA.type == TOK_PLUS) op[0] = '+';
        else op[0] = '-';
        op[1] = '\0';
        match(LA.type);
        char *next_term = parse_term();
        char *temp = new_temp();
        add_quad(op, term_result, next_term, temp);
        term_result = temp;
    }
    return term_result;
}

// 解析项
static char* parse_term(void) {
    char *factor_result = parse_factor();
    
    while (LA.type == TOK_MUL || LA.type == TOK_DIV) {
        char op[2];
        if (LA.type == TOK_MUL) op[0] = '*';
        else op[0] = '/';
        op[1] = '\0';
        match(LA.type);
        char *next_factor = parse_factor();
        char *temp = new_temp();
        add_quad(op, factor_result, next_factor, temp);
        factor_result = temp;
    }
    return factor_result;
}

// 解析因子 — 使用轮转缓冲区避免返回指针被覆盖
static char* parse_factor(void) {
    static char factor_buf[4][16];
    static int buf_idx = 0;
    char *buf = factor_buf[buf_idx];
    buf_idx = (buf_idx + 1) % 4;

    if (LA.type == TOK_IDENT) {
        check_declared(LA.lexeme, LA.line_no);
        strcpy(buf, LA.lexeme);
        match(TOK_IDENT);
        return buf;
    } else if (LA.type == TOK_NUMBER) {
        sprintf(buf, "%d", LA.value);
        match(TOK_NUMBER);
        return buf;
    } else if (LA.type == TOK_LPAREN) {
        match(TOK_LPAREN);
        char *result = parse_expr();
        match(TOK_RPAREN);
        return result;
    } else {
        semantic_error(ERR_SYNTAX, LA.line_no, "表达式错误");
        buf[0] = '0'; buf[1] = '\0';
        return buf;
    }
}

// 语义分析入口
void semantic_analyze(TokenList *token_list) {
    init_symbol_table();
    init_quads();
    semantic_has_error = 0;

    tl = token_list;
    tl->pos = 0;
    parse_program();
    
    if (!semantic_has_error && quad_count > 0) {
        printf("\n✅ 语义分析成功！\n");
        print_symbol_table();
        print_quads();
    } else if (semantic_has_error) {
        printf("\n❌ 语义分析发现错误\n");
    }
}