#ifndef __AHCI_H
#define __AHCI_H

#include <types.h>

// --- AHCI Global HBA Memory Registers (struktur disederhanakan) ---
typedef struct {
    uint32_t cap;        // 0x00, HBA Capabilities
    uint32_t ghc;        // 0x04, Global Host Control
    uint32_t is;         // 0x08, Interrupt Status
    uint32_t pi;         // 0x0C, Ports Implemented
    uint32_t vs;         // 0x10, Version
    uint32_t ccc_ctl;    // 0x14, Command Completion Coalescing Control
    uint32_t ccc_pts;    // 0x18, Command Completion Coalescing Ports
    uint32_t em_loc;     // 0x1C, Enclosure Management Location
    uint32_t em_ctl;     // 0x20, Enclosure Management Control
    uint32_t cap2;       // 0x24, HBA Capabilities Extended
    uint32_t bohc;       // 0x28, BIOS/OS Handoff Control and Status
    uint8_t  reserved[0xA0-0x2C];
    uint8_t  vendor[0x100-0xA0];
    // Port registers mulai dari offset 0x100, array 32 port
} HBA_MEM_t;

// --- AHCI Port Registers (struktur disederhanakan) ---
typedef struct {
    volatile uint32_t clb;       // 0x00, Command List Base Address
    volatile uint32_t clbu;      // 0x04, Command List Base Address Upper 32 bits
    volatile uint32_t fb;        // 0x08, FIS Base Address
    volatile uint32_t fbu;       // 0x0C, FIS Base Address Upper 32 bits
    volatile uint32_t is;        // 0x10, Interrupt Status
    volatile uint32_t ie;        // 0x14, Interrupt Enable
    volatile uint32_t cmd;       // 0x18, Command and Status
    volatile uint32_t rsv0;      // 0x1C, Reserved
    volatile uint32_t tfd;       // 0x20, Task File Data
    volatile uint32_t sig;       // 0x24, Signature
    volatile uint32_t ssts;      // 0x28, SATA Status
    volatile uint32_t sctl;      // 0x2C, SATA Control
    volatile uint32_t serr;      // 0x30, SATA Error
    volatile uint32_t sact;      // 0x34, SATA Active
    volatile uint32_t ci;        // 0x38, Command Issue
    volatile uint32_t sntf;      // 0x3C, SATA Notification
    volatile uint32_t fbs;       // 0x40, FIS-based Switching control
    uint8_t  reserved[0x70-0x44];
} HBA_PORT_t;

#define AHCI_MAX_PORTS 32

// --- AHCI Command List entry (Header) ---
typedef struct {
    volatile uint16_t flags;     // Bits: type, prdt length, etc.
    volatile uint16_t prdtl;     // Physical Region Descriptor Table Length
    volatile uint32_t prdbc;     // Physical Region Descriptor Byte Count transferred
    volatile uint32_t ctba;      // Command Table Descriptor Base Address
    volatile uint32_t ctbau;     // Command Table Descriptor Base Address Upper 32 bits
    volatile uint32_t reserved[4];
}__attribute__((packed)) HBA_CMD_HEADER_t;

// --- AHCI Command Table ---
// Dalam struktur nyata, diikuti oleh PRDT entries.
typedef struct {
    uint8_t  cfis[64];           // Command FIS
    uint8_t  acmd[16];           // ATAPI Command, unused for SATA
    uint8_t  reserved[48];       // Reserved
    // PRDT entries akan diletakkan di sini (untuk contoh sederhana, kita tidak menggunakannya)
}__attribute__((packed)) HBA_CMD_TBL_t;

// Definisi ukuran buffer dan sektor
#define AHCI_SECTOR_SIZE 512

// Perintah ATA yang digunakan dalam FIS
#define ATA_CMD_READ_SECTORS   0x20
#define ATA_CMD_WRITE_SECTORS  0x30

// Deklarasi fungsi
void ahci_init(HBA_MEM_t *abar);
int ahci_read(HBA_PORT_t *port, uint64_t lba, uint32_t sector_count, uint8_t *buffer);
int ahci_write(HBA_PORT_t *port, uint64_t lba, uint32_t sector_count, uint8_t *buffer);

#endif // __AHCI_H
