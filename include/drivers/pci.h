#ifndef __PCI_H
#define __PCI_H

#include <types.h>


// Port PCI Configuration I/O
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC
#define PCI_ENABLE_BIT     0x80000000

// Struktur untuk menyimpan informasi perangkat PCI
typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;    // Base Class (offset 0x0B)
    uint8_t subclass;      // Subclass (offset 0x0A)
    uint8_t prog_if;       // Programming Interface (offset 0x09)
    uint32_t bar[6];
} pci_device_t;

// Declaration
void pci_scan_bus(void);

#endif            //__PCI_H
