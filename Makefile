CC = gcc
CFLAGS = -Wall -O3 -pthread -I./include

all: aics_engine

# 编译主程序 (这里为了演示直接编译测试文件作为入口)
aics_engine: src/policy_rcu.c src/filters.c src/pipeline.c tests/torture_test.c
	$(CC) $(CFLAGS) -o aics_engine $^

clean:
	rm -f aics_engine

test: all
	./aics_engine
