#include "kernel.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include <stdint.h>
#include <stddef.h>

uint16_t* video_mem = 0;
uint16_t ter_row = 0;
uint16_t ter_col = 0;

uint16_t terminal_make_char(char c, char color) {
    return (color << 8) | c;
}

void terminal_putchar(int col, int row, char c, char color) {
    video_mem[(row * VGA_WIDTH) + col] = terminal_make_char(c, color);
}

void terminal_writechar(char c, char color) {
    if (c == '\n') {
        ter_col = 0;
        ter_row += 1;
        return;
    }
    terminal_putchar(ter_col, ter_row, c, color);
    ter_col += 1;
    if (ter_col >= VGA_WIDTH) {
        ter_col = 0;
        ter_row += 1;
    }
}

void terminal_initialize() {
    video_mem = (uint16_t*)(0xb8000);
    for (int row  = 0; row < VGA_HEIGHT; ++row) {
        for (int col = 0; col < VGA_WIDTH; ++col) {
            terminal_putchar(col, row, ' ', 0);
        }
    }
}

size_t strlen(const char* str)
{
    size_t len = 0;
    while(str[len])
    {
        len++;
    }

    return len;
}

void print_kern(const char *str) {
    size_t len = strlen(str);
    for (int index = 0; index < len; ++index) {
        terminal_writechar(str[index], 4);
    }
}

extern void problem();

struct paging_4GB_chunk* kernel_chunk = NULL;

void kernel_main() {
    terminal_initialize();
    print_kern("Welcome to the new world!!\nHello world!!\n");

    //Initialize the heap memory
    kheap_init();

    //Initialize the IDT
    idt_init();

    //Setup paging: Linear Paging: 0x00 points to 0x00 Physical mem and 0x1000 points to 0x1000 Physical mem and so on...
    kernel_chunk = paging_new_4GB(PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);

    //Switch to kernel chunk paging
    //uint32_t* dir = paging_4GB_chunk_get_directory(kernel_chunk);
    paging_switch(paging_4GB_chunk_get_directory(kernel_chunk));

    char *ptr = kzalloc(4096);
    int ret = paging_set(paging_4GB_chunk_get_directory(kernel_chunk), (void*)0x1000, (uint32_t)ptr | PAGING_ACCESS_FROM_ALL | PAGING_IS_PRESENT | PAGING_IS_WRITABLE);
    if (ret < 0) {
        print_kern("paging_set() failed\n");
    }
    //enable paging
    enable_paging();

    //TODO: below code failing as ptr not reflecting the ptr1 data
    char* ptr1 = (char*)0x1000;
    ptr1[0] = 'A';
    ptr1[1] = 'B';
    print_kern(ptr1);
    print_kern(ptr);

    //Enable interrupts: enable interrupts is below than enable paging because paging can cause interrupts 
    enable_interrupts();
}
