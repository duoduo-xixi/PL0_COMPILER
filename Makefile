CC = gcc
CFLAGS = -Wall -g -Iinclude
TARGET = pl0_compiler
GUI_TARGET = pl0_gui

E3_SRC = src/experiment3/e3_tasks.c
SRCS = src/main.c src/lexer.c src/lr_parser.c src/semantic.c $(E3_SRC)
OBJS = $(SRCS:src/%.c=obj/%.o)
GUI_SRCS = src/lexer.c src/lr_parser.c src/semantic.c src/gui.c $(E3_SRC)
GUI_OBJS = $(GUI_SRCS:src/%.c=obj/%.o)

all: $(TARGET) $(GUI_TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(GUI_TARGET): $(GUI_OBJS)
	$(CC) $(CFLAGS) -mwindows -o $(GUI_TARGET) $(GUI_OBJS)

# Windows 兼容：使用 if not exist 代替 mkdir -p
obj/%.o: src/%.c include/common.h src/lexer.h src/lr_parser.h src/semantic.h src/experiment3/e3_tasks.h
	@if not exist $(subst /,\,$(dir $@)) mkdir $(subst /,\,$(dir $@))
	$(CC) $(CFLAGS) -c $< -o $@

# Windows 兼容：使用 rmdir /s /q 和 del 代替 rm -rf
clean:
	@if exist obj rmdir /s /q obj
	@if exist $(TARGET).exe del /Q $(TARGET).exe
	@if exist $(GUI_TARGET).exe del /Q $(GUI_TARGET).exe

# Windows 兼容：去掉 ./ 前缀
run: $(TARGET)
	$(TARGET)

gui: $(GUI_TARGET)
	$(GUI_TARGET)

test: $(TARGET)
	$(TARGET) tests/experiment5_parser/e5_t1_correct.pl0

# Windows 兼容：echo 不加引号
help:
	@echo 可用命令：
	@echo   make        - 编译全部（CLI + GUI）
	@echo   make run    - 运行 CLI 交互式菜单
	@echo   make gui    - 运行图形化界面
	@echo   make test   - 运行正确语法测试
	@echo   make clean  - 清理编译文件

.PHONY: all clean run gui test help