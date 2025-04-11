#ifndef __8259PIC_H
#define __8259PIC_H

#include<types.h>

struct GateDescriptor {
    uint16_t    handlerAddressLowBits;
    uint16_t    gdt_codeSegmentSelector;
    uint8_t     reserved;
    uint8_t     access;
    uint16_t    handlerAddressHighBits;
}__attribute__((packed));

struct InterruptDestciptorPointer {
    uint16_t    size;
    uint32_t    base;
} __attribute__((packed));

// Declaration
int32_t init8259PIC(uint16_t);
void interruptActive(void);
void interruptRun(void);

#endif            //__8259PIC_H
