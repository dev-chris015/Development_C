CC = gcc
CFLAGS = -Wall -Wextra -g

# Archivo/Ejecutable individual a compilar por defecto (modificable con FILE=...)
FILE ?= lab6/ejercicio

# Buscar todos los ejecutables del proyecto (usado para clean y compile-all)
SRCS = $(shell find . -type f -name "*.c" | sed 's|^\./||')
TARGETS = $(SRCS:.c=)

# Por defecto compila ÚNICAMENTE el archivo individual indicado en FILE
all: $(FILE)

# Regla explícita si se desea compilar absolutamente todos los archivos del proyecto
compile-all: $(TARGETS)

# Regla patrón para compilar cualquier archivo .c individualmente
%: %.c
	$(CC) $(CFLAGS) $< -o $@

run: $(FILE)
	./$(FILE)

valgrind: $(FILE)
	valgrind --leak-check=full --show-leak-kinds=all ./$(FILE)

clean:
	rm -f $(TARGETS)

.PHONY: all compile-all run valgrind clean



