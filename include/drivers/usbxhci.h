#ifndef __USBXHCI_H
#define __USBXHCI_H

#include <boolean.h>
#include <types.h>
#include <stdio.h>

#define XHCI_EXT_CAP_ID_LEGACY_SUPPORT 1

// Definisi register USBCMD dan bit reset.
// Register operasional xHCI dimulai pada alamat base + CAPLENGTH.
// USBCMD register berada pada offset 0x00 dari area operasional.
#define USBCMD_OFFSET   0x00
#define USBCMD_HCRST    (1 << 1)   // Bit Host Controller Reset

typedef struct {
    volatile uint8_t caplength;          // 0x00 - CAPLENGTH
    volatile uint8_t reserved;           // 0x01 - Reserved
    volatile uint16_t hci_version;       // 0x02 - HCIVERSION
    volatile uint32_t hcsparams1;        // 0x04 - HCSPARAMS1
    volatile uint32_t hcsparams2;        // 0x08 - HCSPARAMS2
    volatile uint32_t hcsparams3;        // 0x0C - HCSPARAMS3
    volatile uint32_t hccparams1;        // 0x10 - HCCPARAMS1
    volatile uint32_t dboff;             // 0x14 - DBOFF (Doorbell Offset)
    volatile uint32_t rtsoff;            // 0x18 - RTSOFF (Runtime Register Space Offset)
    volatile uint32_t hccparams2;        // 0x1C - HCCPARAMS2 (optional, check HCCPARAMS1 first)
} __attribute__((packed)) xhci_cap_regs_t;

void xhci_print_capability_info(xhci_cap_regs_t* cap_regs_pointer);

// Fungsi pembacaan dan penulisan 32-bit untuk register
static inline uint32_t xhci_read32(volatile uint32_t *addr) {
    return *addr;
}

static inline void xhci_write32(volatile uint32_t *addr, uint32_t value) {
    *addr = value;
}

#endif            // __USBXHCI_H
