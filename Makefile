CC = gcc
CFLAGS = -Wall -g -Iinclude
TARGET = pl0_compiler

SRCS = src/main.c src/lexer.c src/lr_parser.c src/semantic.c
OBJS = $(SRCS:src/%.c=obj/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET) tests/experiment5_parser/e5_t1_correct.pl0

help:
	@echo "可用命令："
	@echo "  make        - 编译程序"
	@echo "  make run    - 运行程序（交互式选择测试）"
	@echo "  make test   - 运行正确语法测试"
	@echo "  make clean  - 清理编译文件"

.PHONY: all clean run test help