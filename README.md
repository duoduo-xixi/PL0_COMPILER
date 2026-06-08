# PL/0 编译器 - 语法分析 + 语义分析

## 📖 项目简介

本项目是**桂林电子科技大学编译原理课程设计**的核心实现，完成了PL/0语言编译器的**语法分析（LR分析）**和**语义分析（中间代码生成）**两大模块。

### 主要功能

| 模块 | 功能 | 输出 |
|------|------|------|
| **词法分析** | 识别Token、处理注释、错误检测 | Token流 |
| **LR语法分析** | 自底向上语法分析、移进/归约序列 | 语法树（隐式） |
| **语义分析** | 符号表管理、四元式生成 | 符号表 + 四元式 |

---

## 📁 项目目录结构

```
pl0_compiler/
│
├── include/                      # 公共头文件目录
│   └── common.h                  # 全局常量、类型定义、结构体
│
├── src/                          # 源代码目录
│   ├── main.c                    # 主程序入口（测试用例管理）
│   ├── lexer.c                   # 词法分析模块
│   ├── lexer.h                   # 词法分析模块头文件
│   ├── lr_parser.c               # LR语法分析模块
│   ├── lr_parser.h               # LR语法分析模块头文件
│   ├── semantic.c                # 语义分析+四元式生成模块
│   └── semantic.h                # 语义分析模块头文件
│
├── tests/                        # 测试用例目录
│   ├── test_correct.pl0          # 正确语法测试（完整PL/0程序）
│   ├── test_error1.pl0           # 错误语法测试（const a := 10）
│   ├── test_error2.pl0           # 错误语法测试（缺少分号）
│   └── test_semantic.pl0         # 语义错误测试（重复声明）
│
├── obj/                          # 编译中间文件（自动生成）
├── output/                       # 输出文件目录（预留）
│
├── Makefile                      # 编译配置文件
└── README.md                     # 项目说明文档（本文件）
```

---

## 🔧 各模块说明

### 1. include/common.h - 公共定义

**作用**：存放所有模块共用的常量、类型和结构体定义。

| 定义 | 说明 |
|------|------|
| `TokenType` | Token类型枚举（保留字、运算符、界符等） |
| `Token` | Token结构体（类型、词素、行号、数值） |
| `Symbol` | 符号表项结构体（名称、类型、值、地址） |
| `Quad` | 四元式结构体（操作符、操作数、结果） |
| 常量 | 最大Token长度、最大标识符长度等 |

### 2. src/lexer.c - 词法分析器

**输入**：PL/0源代码字符串  
**输出**：Token流（通过全局变量`current_token`）

**核心函数**：

| 函数 | 说明 |
|------|------|
| `init_lexer()` | 初始化词法分析器，设置源码指针 |
| `next_token()` | 获取下一个Token，自动跳过空白和注释 |
| `get_token_name()` | 将TokenType转换为字符串 |

**支持的Token类型**：

| 类别 | 示例 |
|------|------|
| 保留字 | const, var, procedure, if, then, while, do, call, read, write, begin, end, odd |
| 标识符 | 字母开头，长度≤8 |
| 数字 | 无符号整数，长度≤8 |
| 运算符 | +, -, *, /, :=, =, #, <, <=, >, >= |
| 界符 | ,, ;, ., (, ) |

**错误处理**：
- 非法字符（如 @, &, !）
- 数字开头的非法标识符（如 2a）
- 标识符/数字长度超限

### 3. src/lr_parser.c - LR语法分析器

**方法**：LR(0) 自底向上语法分析  
**输入**：Token流  
**输出**：移进/归约步骤、语法正确/错误

**核心数据结构**：

| 结构 | 说明 |
|------|------|
| `StateStack` | 状态栈，用于LR分析 |
| `Production` | 产生式定义（左部、右部长度、名称） |
| `action[][]` | ACTION表（状态 × 终结符） |
| `goto_t[][]` | GOTO表（状态 × 非终结符） |

**分析流程**：

```
初始化：状态栈 = [0]
循环：
    state = 栈顶状态
    act = ACTION[state][当前符号]
    if act == 移进：
        将新状态压栈，读下一个Token
    else if act == 归约：
        弹出对应数量的状态
        查GOTO表，将新状态压栈
    else if act == 接受：
        分析成功，退出
```

**支持的文法（部分）**：

```
Program → Block .
Block → ConstDecl VarDecl ProcDecl Stmt
ConstDecl → const ConstDef ; | ε
VarDecl → var IdentList ; | ε
ProcDecl → procedure ident ; Block ; | ε
Stmt → AssignStmt | IfStmt | WhileStmt | CallStmt | ReadStmt | WriteStmt | CompoundStmt | ε
```

### 4. src/semantic.c - 语义分析器

**输入**：语法分析成功的Token流  
**输出**：符号表 + 四元式中间代码

**核心功能**：

| 功能 | 说明 |
|------|------|
| 符号表管理 | 添加/查找符号，检测重复声明 |
| 四元式生成 | 生成(syss)、(syse)、运算、跳转等四元式 |
| 语法驱动分析 | 递归下降遍历语法结构，同步生成代码 |

**四元式类型**：

| 类型 | 示例 | 含义 |
|------|------|------|
| 程序控制 | (syss,_,_,_) | 程序开始 |
| 程序控制 | (syse,_,_,_) | 程序结束 |
| 声明 | (const,a,_,_) | 常量声明 |
| 声明 | (var,b,_,_) | 变量声明 |
| 声明 | (procedure,p,_,_) | 过程声明 |
| 运算 | (+,b,a,T1) | 加法 |
| 运算 | (*,2,c,T1) | 乘法 |
| 赋值 | (:=,T1,_,c) | 赋值 |
| 跳转 | (j#,b,0,$L1) | 条件跳转 |
| I/O | (read,b,_,_) | 输入 |
| I/O | (write,c,_,_) | 输出 |
| 过程 | (call,p,_,_) | 过程调用 |
| 返回 | (ret,_,_,_) | 过程返回 |

**语义错误检测**：
- 重复声明（如 const a=10; var a;）
- 使用未声明的标识符
- 过程未定义

### 5. src/main.c - 主程序

**功能**：
- 支持从文件读取PL/0源代码
- 支持交互式选择测试用例
- 依次执行词法分析、语法分析、语义分析
- 输出分析结果

**运行模式**：

| 模式 | 命令 |
|------|------|
| 文件模式 | `./pl0_compiler tests/test_correct.pl0` |
| 交互模式 | `./pl0_compiler`（然后选择测试用例） |

---

## 🚀 编译与运行

### 环境要求

- 操作系统：Linux / macOS / Windows (MinGW/Cygwin)
- 编译器：GCC 4.8+
- 构建工具：Make

### 编译步骤

```bash
# 1. 进入项目目录
cd pl0_compiler

# 2. 编译
make

# 3. 运行
./pl0_compiler
```

### 运行示例

```
========================================
   PL/0 编译器 - 语法分析 + 语义分析
========================================

选择测试用例:
1. 正确语法测试
2. 错误语法测试 (const a := 10)
3. 语义错误测试 (重复声明)
4. 运行所有测试
请输入选择 (1-4): 1
```

---

## 📊 输出说明

### 词法分析输出

```
✅ 词法分析通过
```

### 语法分析输出

```
========== LR(0) 语法分析过程 ==========
步骤 | 状态栈              | 当前符号     | 动作
-----|---------------------|--------------|----------------
  1 | 0                   | const        | 移进 状态 1
  2 | 0 1                 | ident        | 归约: ConstDecl → const ConstDef ;
  ...
 15 | 0 11 12 13 14 15 20 | $            | 接受

✅ 语法分析成功！程序语法正确
```

### 语义分析输出

```
✅ 语义分析成功！

========== 符号表 ==========
const a = 10
var b
var c

========== 中间代码(四元式) ==========
(1)(syss,_,_,_)
(2)(const,a,_,_)
(3)(=,10,_,a)
(4)(var,b,_,_)
(5)(var,c,_,_)
(6)(read,b,_,_)
(7)(j#,b,0,$L1)
(8)(+,b,a,T1)
(9)(:=,T1,_,c)
(10)(write,c,_,_)
(11)(read,b,_,_)
(12)(j,_,_,$W1)
(13)($WE1,_,_,_)
(14)(syse,_,_,_)
```

---

## 🧪 测试用例说明

### tests/test_correct.pl0 - 正确语法测试

```pascal
const a = 10;
var b, c;
begin
    read(b);
    while b # 0 do
    begin
        c := b + a;
        write(c);
        read(b);
    end
end.
```

### tests/test_error1.pl0 - 错误语法测试

```pascal
const a := 10;    // 错误：应该是 = 而不是 :=
...
```

### tests/test_semantic.pl0 - 语义错误测试

```pascal
const a = 10;
var a, b, c;      // 错误：重复声明 a
...
```

---

## 📝 代码统计

| 文件 | 行数 | 说明 |
|------|------|------|
| common.h | ~80 | 公共定义 |
| lexer.c | ~250 | 词法分析器 |
| lr_parser.c | ~200 | LR语法分析器 |
| semantic.c | ~400 | 语义分析器 |
| main.c | ~120 | 主程序 |
| **总计** | **~1050** | |

---

## 🔄 数据流图

```
PL/0源代码
    │
    ▼
┌─────────────┐
│  词法分析   │  ← lexer.c
│  (next_token)
└──────┬──────┘
       │ Token流
       ▼
┌─────────────┐
│  LR语法分析 │  ← lr_parser.c
│  (移进/归约)
└──────┬──────┘
       │ 语法树(隐式)
       ▼
┌─────────────┐
│  语义分析   │  ← semantic.c
│  (符号表+四元式)
└──────┬──────┘
       │
       ▼
   输出结果
```

---

## 👥 团队分工参考

| 角色 | 负责模块 | 工作量 |
|------|----------|--------|
| 词法分析员 | lexer.c | 25% |
| 语法分析员 | lr_parser.c | 25% |
| 语义分析员 | semantic.c | 35% |
| 集成测试员 | main.c + 测试 | 15% |

---

## 📚 参考资料

1. 《编译原理》（龙书）- Alfred V. Aho
2. PL/0语言规范 - Niklaus Wirth
3. LR分析算法 - Knuth

---

## 📄 许可证

本项目为桂林电子科技大学编译原理课程设计作业，仅供学习交流使用。

---

**作者**：编译原理课程设计小组  
**日期**：2026年6月  
**版本**：1.0


下面是项目的完整讲解。

  ---
  PL/0 编译器 — 完整流程详解

  一、总览：数据如何在模块间流动

    PL/0 源代码 (字符串)
           │
           ▼
    ┌──────────────┐
    │  词法分析器    │  lexer.c — 把字符流切割成 Token 序列
    │  next_token() │  输出: current_token (全局变量，每次一个Token)
    └──────┬───────┘
           │  Token 流
           ▼
    ┌──────────────┐
    │  语法分析器    │  lr_parser.c — 递归下降检查 Token 序列是否符合 PL/0 文法
    │  lr_parse()  │  输出: ParseResult { success, error_line }
    └──────┬───────┘
           │  语法正确
           ▼
    ┌──────────────┐
    │  语义分析器    │  semantic.c — 再次遍历 Token，构建符号表 + 生成四元式
    │semantic_analyze()│  输出: 符号表 (Symbol[]) + 四元式 (Quad[])
    └──────────────┘

  关键设计：三个模块是完全独立的三遍扫描，每一遍都重新初始化词法分析器（init_lexer() + next_token()），各自维护自己的
  lookahead 和 match()。它们之间唯一共享的是词法分析器的全局变量 current_token 和 next_token()。

  ---
  二、main.c — 入口与调度

  2.1 两种运行模式

  ./pl0_compiler                        → 交互式菜单
  ./pl0_compiler tests/xxx.pl0          → 命令行模式，直接完整编译

  2.2 菜单结构

  菜单按实验顺序组织——实验四（词法）→ 实验五（语法）→ 实验六（语义）：

  - [1] → 运行实验四的 2 个测试文件，仅词法分析
  - [2] → 运行实验五的 5 个测试文件，词法 + 语法
  - [3] → 运行实验六的 5 个测试文件，完整三阶段
  - [A] → 按顺序全部跑一遍
  - [C] → 用户输入任意 .pl0 文件路径 + 选择运行模式

  2.3 测试用例的数据结构

  typedef struct {
      const char *filename;
      const char *description;
  } TestCase;

  typedef struct {
      const char *name;
      const char *dir;          // 测试文件所在目录
      const TestCase *tests;    // 测试用例数组
      int count;                // 测试数量
      RunMode mode;             // 该实验对应的运行模式
  } Experiment;

  三个 Experiment 在编译时就定义好了（不是从文件扫描），指向各自的 .pl0 文件。run_experiment() 函数遍历 tests
  数组，逐个调用 run_test_file()，后者读取文件内容为字符串，传给对应模式的运行函数。

  2.4 三种 RunMode

  typedef enum {
      MODE_LEXER_ONLY,      // → run_lexer_only()
      MODE_LEXER_PARSER,    // → run_lexer_and_parser()
      MODE_FULL             // → run_full_compiler()
  } RunMode;

  每种 RunMode 控制核心函数的"组合方式"（后面会展开）。

  ---
  三、lexer.c — 词法分析：如何把字符变成 Token

  3.1 全局状态

  int current_line = 1;        // 当前行号（每遇 \n 自增）
  int has_lex_error = 0;       // 是否遇到过词法错误
  Token current_token;         // 最近一次 next_token() 读到的 Token
  static const char *source;   // 源代码字符串指针
  static int pos;              // 当前位置索引

  init_lexer(src) 把 source 指向源代码、pos 归零、current_line 归 1、清空统计计数器。

  3.2 next_token() — 核心字符扫描机

  这是整个词法分析器最核心的函数，每次调用返回下一个 Token。执行流程如下：

  next_token()
    │
    ├─ 1. skip_whitespace_and_comments()  ← 跳过空白 + 三种注释
    │     如果到达 EOF → current_token.type = TOK_EOF, return
    │
    ├─ 2. 根据 source[pos] 的首字符判断 Token 类型:
    │
    │   ┌─ 字母/_  → 读取完整标识符/关键字
    │   │    ├─ 转小写后查 keywords[] 表
    │   │    ├─ 命中 → 关键字类型 (TOK_CONST, TOK_VAR, ...)
    │   │    ├─ 未命中 → TOK_IDENT
    │   │    └─ 检查长度 ≤ 8，否则报 "标识符长度超长"
    │   │
    │   ├─ 数字 → 读取完整数字串
    │   │    ├─ 如果数字后紧跟字母 (如 2a) → 报 "非法字符(串)"
    │   │    └─ 检查长度 ≤ 8，否则报 "无符号整数越界"
    │   │
    │   ├─ 运算符/界符 → 大 switch(ch) 语句
    │   │    ├─ 单字符: + - * / = # , ; . ( )
    │   │    ├─ 双字符前瞻: <=, <>, >=, :=
    │   │    │   原理: 读 ch 后再 peek source[pos+1]
    │   │    │   例如 ':' → 如果下一字符是 '=' → TOK_ASSIGN (:=)
    │   │    │              → 否则 → TOK_ERROR (单独的 : 非法)
    │   │    └─ 非法字符 → TOK_ERROR + 报 "非法字符"
    │   │
    └─ 3. token_count_by_type[type]++  ← 统计每种 Token 出现次数

  3.3 注释处理 (skip_whitespace_and_comments)

  三种注释风格：

  ┌───────┬─────────────┬─────────────────────────┐
  │ 格式  │    示例     │        处理方式         │
  ├───────┼─────────────┼─────────────────────────┤
  │ //    │ 单行注释    │ 读到换行符为止          │
  ├───────┼─────────────┼─────────────────────────┤
  │ /* */ │ C 风格多行  │ 读到 */，中间换行要计数 │
  ├───────┼─────────────┼─────────────────────────┤
  │ (* *) │ Pascal 风格 │ 同 /* */，读到 *)       │
  └───────┴─────────────┴─────────────────────────┘

  遇到 // 时，跳过 // 然后持续读字符直到 \n。遇到 /* 或 (* 时，持续扫描直到对应的闭合标记，途中的 \n 增加
  current_line。如果到文件末尾还没闭合，报 "多行注释未闭合"。

  3.4 全局状态的问题——为什么每遍都要重新 init_lexer

  current_token 是全局变量，next_token() 每次调用会覆盖它。这意味着任何时候只有一个"当前 Token"，而且 Token
  流是单向消费的——一旦调用 next_token() 就无法回到之前的 Token。

  所以每个模块如果要完整遍历一遍 Token 流，必须：
  1. init_lexer(source) — 重置 source 和 pos
  2. next_token() — 读到第一个 Token
  3. 然后循环消费所有 Token

  这就是为什么 run_full_compiler() 里调用了三次 init_lexer()——词法输出一次、语法分析一次、语义分析一次。

  3.5 输出格式

  print_token_for_lexer() 按任务书要求输出：
  (保留字,const)
  (标识符,a)
  (运算符,:=)
  (无符号整数,10)
  (界符,;)

  ---
  四、lr_parser.c — 语法分析：递归下降法

  4.1 总览

  虽然叫 "lr_parser"，但实际上是递归下降（recursive descent）解析器——每个非终结符对应一个 parse_xxx()
  函数，函数之间相互递归调用，与 PL/0 的 EBNF 文法一一对应。

  parse_program()     → <程序>      ::= <分程序> .
  parse_block()       → <分程序>    ::= [常量声明] [变量声明] [过程声明] <语句>
  parse_const_decl()  → <常量说明部分> ::= const <常量定义> {,<常量定义>};
  parse_var_decl()    → <变量说明部分> ::= var <标识符> {,<标识符>};
  parse_proc_decl()   → <过程说明部分> ::= <过程首部><分程序> {;<过程说明部分>};
  parse_statement()   → <语句>      ::= 赋值|if|while|call|read|write|begin|空
  parse_condition()   → <条件>      ::= <表达式> <关系运算符> <表达式> | odd <表达式>
  parse_expression()  → <表达式>    ::= [+|-]<项> {<加减运算符><项>}
  parse_term()        → <项>        ::= <因子> {<乘除运算符><因子>}
  parse_factor()      → <因子>      ::= <标识符>|<无符号整数>|(<表达式>)

  4.2 match() 函数 — 核心机制

  static void match(TokenType expected) {
      if (lookahead.type == expected) {
          next_token();             // 匹配成功 → 消费此Token，读下一个
          lookahead = current_token;
      } else {
          parse_has_error = 1;
          printf("(语法错误,行号:%d)\n", lookahead.line_no);
          // 错误恢复：跳过Token直到同步符号
          while (lookahead.type != TOK_SEMI && ...) {
              next_token();
              lookahead = current_token;
          }
      }
  }

  匹配成功：消费当前 Token，前进到下一个。

  匹配失败：报语法错误（带行号），然后进入错误恢复模式——不断跳过 Token，直到遇到同步符号（; end begin .
  EOF），这避免了"一错就停"，让解析器能继续检查后面的代码。

  4.3 parse_statement() — 语句分发

  这是解析器的"调度中心"，根据 lookahead.type 决定走哪个分支：

  TOK_IDENT → 赋值语句   (a := expr)
  TOK_IF    → 条件语句   (if cond then stmt)
  TOK_WHILE → 循环语句   (while cond do stmt)
  TOK_CALL  → 过程调用   (call ident)
  TOK_READ  → 读语句     (read(ident, ...))
  TOK_WRITE → 写语句     (write(expr, ...))
  TOK_BEGIN → 复合语句   (begin stmt; stmt; ... end)
  其他      → 空语句 (ε)

  注意：这里的语法分析器只检查"结构对不对"，不做任何语义检查——它不判断变量名是否声明过、类型是否匹配。那些留给语义分析。

  4.4 lr_parse() — 入口

  ParseResult lr_parse() {
      parse_has_error = 0;
      lookahead = current_token;   // 从词法分析器的当前Token开始
      parse_program();
      // parse_has_error 在 match() 失败时被设置为 1
      if (!parse_has_error) printf("语法正确\n");
      return result;
  }

  ---
  五、semantic.c — 语义分析：符号表 + 四元式

  5.1 与语法分析器的关系

  语义分析器重新实现了一套递归下降解析器，和 lr_parser.c 结构几乎相同——同样的
  parse_program()、parse_block()、parse_statement() 函数名（用 static 限定在同文件内）。区别在于语义分析器的每个
  parse_xxx() 不仅做语法检查，还同时执行语义动作（建符号表、生成四元式）。

  5.2 符号表 (Symbol Table)

  Symbol symbol_table[MAX_SYMBOL_TABLE];
  int symbol_count = 0;

  每个 Symbol 包含：name（标识符名）、type（CONST/VAR/PROC）、value（常量的值）、level（作用域）、addr（地址）。

  - add_symbol() — 先遍历现有符号表检查是否重名，重复则返回 0（触发 ERR_SEM_DUPLICATE）
  - find_symbol() — 从后往前查（后面声明的优先，支持作用域遮蔽）

  5.3 四元式 (Quad)

  Quad quads[MAX_QUAD_SIZE];
  int quad_count = 0;

  每个 Quad = { op, arg1, arg2, result }，例如 (+, b, a, T1) 表示 T1 = b + a。

  add_quad() 往数组添加一条，返回当前索引。临时变量 T1, T2, ... 由 new_temp() 生成（计数器 temp_counter 自增）。

  5.4 类型检查的三个函数（最近修复的核心）

  check_declared(name, line)
    └─ find_symbol(name) == NULL? → 报 "未声明"

  check_is_variable(name, line)    ← 用于 read(ident) 和 ident := expr
    ├─ find_symbol(name) == NULL? → 报 "未声明"
    └─ sym->type != SYM_VAR?      → 报 "类型不匹配"  (如 read(p) 时 p 是 procedure)

  check_is_procedure(name, line)   ← 用于 call ident
    ├─ find_symbol(name) == NULL? → 报 "未声明"
    └─ sym->type != SYM_PROC?     → 报 "类型不匹配"  (如 call 一个变量)

  调用位置：

  ┌──────────────────────────┬───────────────────────────────────────┐
  │         语句类型         │            调用的检查函数             │
  ├──────────────────────────┼───────────────────────────────────────┤
  │ ident := expr (赋值左值) │ check_is_variable                     │
  ├──────────────────────────┼───────────────────────────────────────┤
  │ read(ident)              │ check_is_variable                     │
  ├──────────────────────────┼───────────────────────────────────────┤
  │ call ident               │ check_is_procedure                    │
  ├──────────────────────────┼───────────────────────────────────────┤
  │ 因子中的 ident（读值）   │ check_declared（const 和 var 都可以） │
  └──────────────────────────┴───────────────────────────────────────┘

  5.5 四元式生成过程（以具体代码为例）

  输入：
  const a = 10;
  var b, c;
  begin
      read(b);
      while b # 0 do
          c := b + a;
  end.

  执行路径：

  parse_program()
    add_quad("syss", ...)                  → (1)  syss
    parse_block()
      parse_const_decl()                   → (2) const,a  (3) =,10,_,a
      parse_var_decl()                     → (4) var,b    (5) var,c
      parse_proc_decl() — 没有 procedure，跳过
      parse_statement()
        → 遇到 TOK_BEGIN → parse_statement()...
           TOK_READ → add_quad("read","b") → (7) read,b
           TOK_WHILE:
             add_quad("$W1",...)           → (8) $W1
             parse_condition() — b # 0
               → add_quad("j#","b","0","$WE1") → (9) j#,b,0,$WE1
             parse_statement() — c := b + a
               check_is_variable("c")      → c 是 SYM_VAR ✓
               parse_expr() → parse_term() → parse_factor("b") → "b"
                            → + → parse_factor("a") → new_temp()="T1"
                            → add_quad("+","b","a","T1") → (10) +,b,a,T1
               add_quad(":=", "T1", "_", "c") → (11) :=,T1,_,c
             add_quad("j","_","_","$W1")    → (12) j,_,_,$W1
             add_quad("$WE1",...)           → (13) $WE1
    add_quad("syse", ...)                  → (14) syse

  5.6 语义分析入口

  void semantic_analyze(void) {
      init_symbol_table();
      init_quads();
      lookahead = current_token;
      parse_program();
      if (!semantic_has_error) {
          print_symbol_table();
          print_quads();
      } else {
          printf("❌ 语义分析发现错误\n");
      }
  }

  ---
  六、完整运行流程（以 make run → 选 [3] 实验六为例）

  1. main() → show_menu() → 用户输入 '3'
  2. main() → run_experiment(&experiments[2])
  3. 对 experiment6_semantic/ 下 5 个测试文件逐一:
     run_test_file(filepath, MODE_FULL)
       ├─ read_file() → 读 .pl0 → 得到 source_code 字符串
       ├─ run_full_compiler(source_code):
       │   ├─ init_lexer() + 第一遍 next_token() 循环 → 输出全部Token + 统计
       │   ├─ init_lexer() 重置 + lr_parse() → 递归下降语法检查
       │   └─ init_lexer() 重置 + semantic_analyze() → 符号表 + 四元式
       └─ free(source_code)
