.set IRQ_BASE , 0x20

.section .text

.extern handleInterrupt

.macro  HandleInterruptRequest num name
.global HandleInterruptRequest\name
HandleInterruptRequest\name:
    movb $\num+IRQ_BASE,(interruptnumber)
    jmp int_bottom
.endm

.macro  HandleException num
.global HandleException\num\()Ev
    movb $\num + IRQ_BASE,(interruptnumber)
    jmp int_bottom
.endm

HandleInterruptRequest 00 00
HandleInterruptRequest 01 01
HandleInterruptRequest 02 02
HandleInterruptRequest 0x0C 0C

int_bottom:
    pusha
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    pushl %esp
    push (interruptnumber)
    call  handleInterrupt
    movl %eax,%esp

    popl %gs
    popl %fs
    popl %es
    popl %ds
    popa

    iret

.data
    interruptnumber : .byte 0
