#include "lr_parser.h"
#include <stdio.h>
#include <string.h>

// 递归下降语法分析器 — 实验五

static TokenList *tl;
#define LA (tl->tokens[tl->pos])

static int parse_has_error = 0;
static int panic_mode = 0;
static int last_matched_line = 1;

// 前向声明
static void parse_program(void);
static void parse_block(void);
static void parse_const_decl(void);
static void parse_var_decl(void);
static void parse_proc_decl(void);
static void parse_statement(void);
static void parse_condition(void);
static void parse_expression(void);
static void parse_term(void);
static void parse_factor(void);

// 匹配当前Token并前进
static void match(TokenType expected) {
    if (LA.type == expected) {
        if (LA.type != TOK_EOF) {
            last_matched_line = LA.line_no;
            tl->pos++;
        }
        panic_mode = 0;
    } else {
        if (!panic_mode) {
            parse_has_error = 1;
            printf("(语法错误,行号:%d)\n", LA.line_no);
            panic_mode = 1;
        }
        // 错误恢复：跳过Token直到找到同步符号
        while (LA.type != TOK_SEMI &&
               LA.type != TOK_END &&
               LA.type != TOK_BEGIN &&
               LA.type != TOK_EOF &&
               LA.type != TOK_PERIOD) {
            if (LA.type != TOK_EOF) tl->pos++;
        }
        // 如果恢复到的同步符号恰好是期望的符号，消费它并退出panic
        if (LA.type == expected) {
            if (LA.type != TOK_EOF) {
                last_matched_line = LA.line_no;
                tl->pos++;
            }
            panic_mode = 0;
        }
    }
}

// <程序> ::= <分程序> .
static void parse_program(void) {
    parse_block();
    match(TOK_PERIOD);
    if (LA.type != TOK_EOF && !panic_mode) {
        parse_has_error = 1;
        printf("(语法错误,行号:%d)\n", LA.line_no);
    }
}

// <分程序> ::= [<常量说明部分>][<变量说明部分>][<过程说明部分>]<语句>
static void parse_block(void) {
    parse_const_decl();
    parse_var_decl();
    parse_proc_decl();
    parse_statement();
}

// <常量说明部分> ::= const <常量定义>{,<常量定义>};
static void parse_const_decl(void) {
    if (LA.type == TOK_CONST) {
        match(TOK_CONST);
        do {
            match(TOK_IDENT);
            match(TOK_EQ);
            match(TOK_NUMBER);
        } while (LA.type == TOK_COMMA && (match(TOK_COMMA), 1));
        match(TOK_SEMI);
    }
}

// <变量说明部分> ::= var <标识符>{,<标识符>};
static void parse_var_decl(void) {
    if (LA.type == TOK_VAR) {
        match(TOK_VAR);
        do {
            match(TOK_IDENT);
        } while (LA.type == TOK_COMMA && (match(TOK_COMMA), 1));
        match(TOK_SEMI);
    }
}

// <过程说明部分> ::= <过程首部><分程序>{;<过程说明部分>};
static void parse_proc_decl(void) {
    while (LA.type == TOK_PROCEDURE) {
        match(TOK_PROCEDURE);
        match(TOK_IDENT);
        match(TOK_SEMI);
        parse_block();
        match(TOK_SEMI);
    }
}

// <语句> ::= <赋值语句>|<条件语句>|<当型循环语句>
//           |<过程调用语句>|<读语句>|<写语句>|<复合语句>|<空语句>
static void parse_statement(void) {
    switch (LA.type) {
        case TOK_IDENT:
            // <赋值语句> ::= <标识符> := <表达式>
            match(TOK_IDENT);
            match(TOK_ASSIGN);
            parse_expression();
            break;
        case TOK_IF:
            // <条件语句> ::= if <条件> then <语句>
            match(TOK_IF);
            parse_condition();
            match(TOK_THEN);
            parse_statement();
            break;
        case TOK_WHILE:
            // <当型循环语句> ::= while <条件> do <语句>
            match(TOK_WHILE);
            parse_condition();
            match(TOK_DO);
            parse_statement();
            break;
        case TOK_CALL:
            // <过程调用语句> ::= call <标识符>
            match(TOK_CALL);
            match(TOK_IDENT);
            break;
        case TOK_READ:
            // <读语句> ::= read ( <标识符>{,<标识符>} )
            match(TOK_READ);
            match(TOK_LPAREN);
            match(TOK_IDENT);
            while (LA.type == TOK_COMMA) {
                match(TOK_COMMA);
                match(TOK_IDENT);
            }
            match(TOK_RPAREN);
            break;
        case TOK_WRITE:
            // <写语句> ::= write ( <表达式>{,<表达式>} )
            match(TOK_WRITE);
            match(TOK_LPAREN);
            parse_expression();
            while (LA.type == TOK_COMMA) {
                match(TOK_COMMA);
                parse_expression();
            }
            match(TOK_RPAREN);
            break;
        case TOK_BEGIN:
            // <复合语句> ::= begin <语句>{;<语句>} end
            match(TOK_BEGIN);
            parse_statement();
            while (LA.type == TOK_SEMI) {
                match(TOK_SEMI);
                parse_statement();
            }
            match(TOK_END);
            break;
        default:
            // <空语句> — 允许空
            break;
    }
}

// <条件> ::= <表达式> <关系运算符> <表达式> | odd <表达式>
static void parse_condition(void) {
    if (LA.type == TOK_ODD) {
        match(TOK_ODD);
        parse_expression();
    } else {
        parse_expression();
        // 关系运算符
        if (LA.type == TOK_EQ || LA.type == TOK_NEQ ||
            LA.type == TOK_LT || LA.type == TOK_LE ||
            LA.type == TOK_GT || LA.type == TOK_GE) {
            match(LA.type);
        } else {
            if (!panic_mode) {
                parse_has_error = 1;
                printf("(语法错误,行号:%d)\n", LA.line_no);
                panic_mode = 1;
            }
        }
        parse_expression();
    }
}

// <表达式> ::= [+|-]<项>{<加减运算符><项>}
static void parse_expression(void) {
    if (LA.type == TOK_PLUS || LA.type == TOK_MINUS) {
        match(LA.type);
    }
    parse_term();
    while (LA.type == TOK_PLUS || LA.type == TOK_MINUS) {
        match(LA.type);
        parse_term();
    }
}

// <项> ::= <因子>{<乘除运算符><因子>}
static void parse_term(void) {
    parse_factor();
    while (LA.type == TOK_MUL || LA.type == TOK_DIV) {
        match(LA.type);
        parse_factor();
    }
}

// <因子> ::= <标识符> | <无符号整数> | ( <表达式> )
static void parse_factor(void) {
    switch (LA.type) {
        case TOK_IDENT:
            match(TOK_IDENT);
            break;
        case TOK_NUMBER:
            match(TOK_NUMBER);
            break;
        case TOK_LPAREN:
            match(TOK_LPAREN);
            parse_expression();
            match(TOK_RPAREN);
            break;
        default:
            if (!panic_mode) {
                parse_has_error = 1;
                printf("(语法错误,行号:%d)\n", LA.line_no);
                panic_mode = 1;
            }
            break;
    }
}

ParseResult lr_parse(TokenList *token_list) {
    ParseResult result;
    result.success = 1;
    result.error_msg[0] = '\0';
    result.error_line = 0;

    parse_has_error = 0;
    panic_mode = 0;
    last_matched_line = 1;
    tl = token_list;
    tl->pos = 0;

    printf("递归下降语法分析过程\n");
    printf("--------------------\n");

    parse_program();

    if (parse_has_error) {
        result.success = 0;
    } else {
        printf("语法正确\n");
    }

    return result;
}
