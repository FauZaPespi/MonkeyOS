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

# Assemble boot code and ISR stubs
RUN nasm -f elf32 boot.asm -o boot.o && \
    nasm -f elf32 cpu/isr.asm -o isr.o

# Compile C files with include path
# Core kernel files
RUN gcc -m32 -c kernel.c -o kernel.o -Iinclude -ffreestanding -nostdlib -fno-stack-protector -fno-pie && \
    gcc -m32 -c drivers/vga.c -o vga.o -Iinclude -ffreestanding -nostdlib -fno-stack-protector -fno-pie && \
    gcc -m32 -c drivers/pic.c -o pic.o -Iinclude -ffreestanding -nostdlib -fno-stack-protector -fno-pie && \
    gcc -m32 -c drivers/keyboard.c -o keyboard.o -Iinclude -ffreestanding -nostdlib -fno-stack-protector -fno-pie && \
    gcc -m32 -c drivers/ata.c -o ata.o -Iinclude -ffreestanding -nostdlib -fno-stack-protector -fno-pie && \
    gcc -m32 -c cpu/idt.c -o idt.o -Iinclude -ffreestanding -nostdlib -fno-stack-protector -fno-pie

# New files: string utilities, filesystem, shell, editor
RUN gcc -m32 -c lib/string.c -o string.o -Iinclude -ffreestanding -nostdlib -fno-stack-protector -fno-pie && \
    gcc -m32 -c fs/fs.c -o fs.o -Iinclude -ffreestanding -nostdlib -fno-stack-protector -fno-pie && \
    gcc -m32 -c shell/shell.c -o shell.o -Iinclude -ffreestanding -nostdlib -fno-stack-protector -fno-pie && \
    gcc -m32 -c shell/commands.c -o commands.o -Iinclude -ffreestanding -nostdlib -fno-stack-protector -fno-pie && \
    gcc -m32 -c apps/editor.c -o editor.o -Iinclude -ffreestanding -nostdlib -fno-stack-protector -fno-pie

# Link everything together
RUN ld -m elf_i386 -T linker.ld -o kernel \
    boot.o isr.o kernel.o vga.o pic.o keyboard.o ata.o idt.o \
    string.o fs.o shell.o commands.o editor.o

# When we run the container, it will just verify the file exists
CMD ["ls", "-la", "kernel"]
