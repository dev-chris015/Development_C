# Compiler and flags configuration
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -g -pthread
LDFLAGS ?= -pthread

# Automatically find all .c files in subdirectories (up to 4 levels deep)
SRCS := $(shell find . -maxdepth 4 -type f -name "*.c" ! -path '*/.*')

# Generate executable target paths without the .c extension
EXECS := $(SRCS:.c=)

.PHONY: all clean help list

# Default target: compile all found C files
all: $(EXECS)
	@echo "=========================================="
	@echo " Compilación finalizada con éxito."
	@echo "=========================================="

# Generic rule to compile any .c file into its executable target
%: %.c
	@echo "[CC] Compilando $< ..."
	@$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Clean up generated executable files
clean:
	@echo "Limpiando ejecutables..."
	@for exec in $(EXECS); do \
		if [ -f "$$exec" ]; then \
			rm -f "$$exec"; \
			echo "Eliminado: $$exec"; \
		fi \
	done
	@echo "Limpieza completada."

# List all C source files found
list:
	@echo "Archivos C encontrados en las subcarpetas:"
	@for src in $(SRCS); do \
		echo " - $$src"; \
	done

# Help menu
help:
	@echo "=========================================================="
	@echo " Makefile para Development_C"
	@echo "=========================================================="
	@echo " Comandos disponibles:"
	@echo "   make        - Compila automáticamente todos los .c en subcarpetas"
	@echo "   make clean  - Elimina todos los ejecutables compilados"
	@echo "   make list   - Muestra la lista de todos los archivos .c detectados"
	@echo "   make help   - Muestra este menú de ayuda"
	@echo "=========================================================="
