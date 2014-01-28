SRCS = $(shell find . -name "*.c")
BCS = $(SRCS:.c=.bc)

CC = "/pool/users/pc424/llvm_build/bin/clang"

ir: $(BCS)

%.bc: %.c
	@$(CC) -S -emit-llvm $< -o $@
	@echo "Compiling $< to LLVM IR"
