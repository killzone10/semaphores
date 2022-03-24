all: main.c sync_var.c
	gcc  -pthread  $^ -lm
