CC = gcc
CFLAGS = -Wall -Wextra -g

# Buscar todos los archivos .c en el proyecto (removiendo el prefijo ./)
SRCS = $(shell find . -type f -name "*.c" | sed 's|^\./||')
TARGETS = $(SRCS:.c=)

# Ejecutable por defecto si no se indica FILE=... (ej: lab6/ejercicio)
FILE ?= lab6/ejercicio

all: $(TARGETS)

# Regla patrón general para compilar cualquier archivo .c
%: %.c
	$(CC) $(CFLAGS) $< -o $@

run: $(FILE)
	./$(FILE)

valgrind: $(FILE)
	valgrind --leak-check=full --show-leak-kinds=all ./$(FILE)

clean:
	rm -f $(TARGETS)

.PHONY: all run valgrind clean


