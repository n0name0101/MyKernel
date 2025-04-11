.set IRQ_BASE , 0x20

.section .text

.extern handleInt8259

.macro  HandleInterruptRequest num name
.global HandleInterruptRequest\name
HandleInterruptRequest\name:
    movb $\num,(interruptnumber)
    jmp int_bottom
.endm

.macro  HandleException num name
.global HandleException\name
HandleException\name:
    movb $\num,(interruptnumber)
    jmp int_bottom
.endm

// CPU Exception
HandleException 0x00 00
HandleException 0x01 01
HandleException 0x02 02
HandleException 0x03 03
HandleException 0x04 04
HandleException 0x05 05
HandleException 0x06 06
HandleException 0x07 07
HandleException 0x08 08
HandleException 0x09 09
HandleException 0x0A 0A
HandleException 0x0B 0B
HandleException 0x0C 0C
HandleException 0x0D 0D
HandleException 0x0E 0E
HandleException 0x0F 0F

HandleException 0x10 10
HandleException 0x11 11
HandleException 0x12 12
HandleException 0x13 13
HandleException 0x14 14
HandleException 0x15 15
HandleException 0x16 16
HandleException 0x17 17
HandleException 0x18 18
HandleException 0x19 19
HandleException 0x1A 1A
HandleException 0x1B 1B
HandleException 0x1C 1C
HandleException 0x1D 1D
HandleException 0x1E 1E
HandleException 0x1F 1F

HandleInterruptRequest 0x30 30         #Unknown Interrupt

// 8259 PIC
HandleInterruptRequest 0x20 20
HandleInterruptRequest 0x21 21
HandleInterruptRequest 0x22 22
HandleInterruptRequest 0x23 23
HandleInterruptRequest 0x24 24
HandleInterruptRequest 0x25 25
HandleInterruptRequest 0x26 26
HandleInterruptRequest 0x27 27
HandleInterruptRequest 0x28 28
HandleInterruptRequest 0x29 29
HandleInterruptRequest 0x2A 2A
HandleInterruptRequest 0x2B 2B
HandleInterruptRequest 0x2C 2C
HandleInterruptRequest 0x2D 2D
HandleInterruptRequest 0x2E 2E
HandleInterruptRequest 0x2F 2F

int_bottom:
    pusha
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    pushl %esp
    push (interruptnumber)
    call  handleInt8259
    movl %eax,%esp

    popl %gs
    popl %fs
    popl %es
    popl %ds
    popa

    iret

.data
    interruptnumber : .byte 0
