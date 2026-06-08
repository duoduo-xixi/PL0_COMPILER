CC = gcc
CFLAGS = -Wall -g -Iinclude
TARGET = pl0_compiler
GUI_TARGET = pl0_gui

SRCS = src/main.c src/lexer.c src/lr_parser.c src/semantic.c
OBJS = $(SRCS:src/%.c=obj/%.o)
GUI_SRCS = src/lexer.c src/lr_parser.c src/semantic.c src/gui.c
GUI_OBJS = $(GUI_SRCS:src/%.c=obj/%.o)

all: $(TARGET) $(GUI_TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(GUI_TARGET): $(GUI_OBJS)
	$(CC) $(CFLAGS) -mwindows -o $(GUI_TARGET) $(GUI_OBJS)

obj/%.o: src/%.c include/common.h src/lexer.h src/lr_parser.h src/semantic.h
	@-mkdir obj 2>nul
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj $(TARGET) $(GUI_TARGET).exe

run: $(TARGET)
	./$(TARGET)

gui: $(GUI_TARGET)
	./$(GUI_TARGET)

test: $(TARGET)
	./$(TARGET) tests/experiment5_parser/e5_t1_correct.pl0

help:
	@echo "可用命令："
	@echo "  make        - 编译全部（CLI + GUI）"
	@echo "  make run    - 运行 CLI 交互式菜单"
	@echo "  make gui    - 运行图形化界面"
	@echo "  make test   - 运行正确语法测试"
	@echo "  make clean  - 清理编译文件"

.PHONY: all clean run gui test help