# Use a lightweight Linux base
FROM ubuntu:latest

# Prevent prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Install the tools: nasm (assembler), gcc/binutils (compiler/linker)
RUN apt-get update && apt-get install -y \
    nasm \
    gcc-multilib \
    binutils \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container
WORKDIR /os-build

# Copy your source code into the container
COPY . .

# Run the build commands
RUN nasm -f elf32 boot.asm -o kasm.o && \
    gcc -m32 -c kernel.c -o kc.o -ffreestanding -nostdlib -fno-stack-protector && \
    ld -m elf_i386 -T linker.ld -o kernel kasm.o kc.o

# When we run the container, it will just verify the file exists
CMD ["ls", "-l", "kernel"]