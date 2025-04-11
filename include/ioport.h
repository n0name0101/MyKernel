#ifndef __IOPORT_H
#define __IOPORT_H

#include <types.h>

// Declaration
static inline void outb(uint16_t _portNum , uint8_t _data) {
    asm __volatile__ ("outb %0, %1" : : "a"(_data) , "Nd" (_portNum));
}

static inline uint8_t inb(uint16_t _portNum) {
    uint8_t result = 0;
    asm __volatile__ ("inb %1,%0" : "=a" (result) : "Nd" (_portNum));
    return result;
}

static inline void outw(uint16_t port, uint16_t data) {
    __asm__ __volatile__ ("outw %0, %1" : : "a"(data), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t data;
    __asm__ __volatile__ ("inw %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

static inline void outl(uint16_t _portNum , uint32_t _data) {
    asm __volatile__ ("outl %0, %1" : : "a"(_data) , "Nd" (_portNum));
}

static inline uint32_t inl(uint16_t _portNum) {
    uint32_t result = 0;
    asm __volatile__ ("inl %1,%0" : "=a" (result) : "Nd" (_portNum));
    return result;
}

// Fungsi delay pendek untuk memberikan waktu bagi hardware melakukan proses I/O
static inline void io_wait(void) {
    __asm__ __volatile__ ("outb %%al, $0x80" : : "a"(0));
}

#endif            //__IOPORT_H
