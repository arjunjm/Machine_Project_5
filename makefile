CPP = gcc
CPP_OPTIONS = -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-exceptions -fno-rtti -fno-stack-protector -fleading-underscore -m32

all: kernel.bin

clean:
	rm -f *.o

start.o: start.asm gdt_low.asm idt_low.asm irq_low.asm
	nasm -f elf -o start.o start.asm

utils.o: utils.C utils.H
	$(CPP) $(CPP_OPTIONS) -c -o utils.o utils.C

assert.o: assert.C assert.H
	$(CPP) $(CPP_OPTIONS) -c -o assert.o assert.C

gdt.o: gdt.C gdt.H
	$(CPP) $(CPP_OPTIONS) -c -o gdt.o gdt.C

# ==== MACHINE =====

machine.o: machine.C machine.H
	$(CPP) $(CPP_OPTIONS) -c -o machine.o machine.C

# ==== EXCEPTIONS AND INTERRUPTS =====

idt.o: idt.C idt.H
	$(CPP) $(CPP_OPTIONS) -c -o idt.o idt.C

irq.o: irq.C irq.H
	$(CPP) $(CPP_OPTIONS) -c -o irq.o irq.C

exceptions.o: exceptions.C exceptions.H
	$(CPP) $(CPP_OPTIONS) -c -o exceptions.o exceptions.C

interrupts.o: interrupts.C interrupts.H
	$(CPP) $(CPP_OPTIONS) -c -o interrupts.o interrupts.C


# ==== DEVICES =====

console.o: console.C console.H
	$(CPP) $(CPP_OPTIONS) -c -o console.o console.C

simple_timer.o: simple_timer.C simple_timer.H
	$(CPP) $(CPP_OPTIONS) -c -o simple_timer.o simple_timer.C

simple_disk.o: simple_disk.C simple_disk.H
	$(CPP) $(CPP_OPTIONS) -c -o simple_disk.o simple_disk.C

blocking_disk.o: blocking_disk.C blocking_disk.H
	$(CPP) $(CPP_OPTIONS) -c -o blocking_disk.o blocking_disk.C

# ==== MEMORY =====

frame_pool.o: frame_pool.C frame_pool.H 
	$(CPP) $(CPP_OPTIONS) -c -o frame_pool.o frame_pool.C

mem_pool.o: mem_pool.C mem_pool.H 
	$(CPP) $(CPP_OPTIONS) -c -o mem_pool.o mem_pool.C

# ==== THREADS =====

threads_low.o: threads_low.asm threads_low.H
	nasm -f elf -o threads_low.o threads_low.asm

thread.o: thread.C thread.H threads_low.H
	$(CPP) $(CPP_OPTIONS) -c -o thread.o thread.C

# ==== MAIN =====

Scheduler.o: Scheduler.C Scheduler.H
	$(CPP) $(CPP_OPTIONS) -c -o Scheduler.o Scheduler.C

kernel.o: kernel.C console.H simple_timer.H
	$(CPP) $(CPP_OPTIONS) -c -o kernel.o kernel.C

kernel.bin: start.o utils.o kernel.o assert.o console.o gdt.o idt.o irq.o machine.o exceptions.o interrupts.o \
   simple_timer.o simple_disk.o frame_pool.o threads_low.o thread.o Scheduler.o mem_pool.o blocking_disk.o
	ld -melf_i386 -T linker.ld -o kernel.bin start.o utils.o kernel.o assert.o console.o gdt.o idt.o \
   exceptions.o irq.o machine.o interrupts.o simple_timer.o simple_disk.o frame_pool.o threads_low.o thread.o Scheduler.o mem_pool.o blocking_disk.o
	
