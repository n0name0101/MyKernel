#ifndef __ATA_H
#define __ATA_H

#include <types.h>

// Definisi channel ATA (Primary dan Secondary)
typedef struct {
   uint16_t io_base;    // Base I/O address untuk channel
   uint16_t ctrl_base;  // Control base address
   const char* name;    // Nama channel (contoh: "Primary", "Secondary")
} ata_channel_t;

// Struktur untuk menyimpan informasi drive ATA yang terdeteksi
typedef struct {
    ata_channel_t *channelPointer;
    uint8_t drive;            // 0xA0 untuk Master, 0xB0 untuk Slave
    char    model[41];        // Model drive (maks. 40 karakter + null terminator)
    uint32_t total_sectors;   // Total jumlah sektor (biasanya dari kata 60-61 IDENTIFY)
} ata_drive_t;

#define NUM_CHANNELS                2
#define NUM_DRIVES_PER_CHANNEL      2
#define MAX_ATA_DRIVES              (NUM_CHANNELS * NUM_DRIVES_PER_CHANNEL)

// --- ATA Registers Offset ---
#define ATA_REG_DATA      0x00            // Data Register
#define ATA_REG_ERROR     0x01            // Error Register (read) / Features (write)
#define ATA_REG_FEATURES  0x01            // Features (write)
#define ATA_REG_SECCOUNT0 0x02            // Sector Count Register
#define ATA_REG_LBA0      0x03            // LBA Low Register
#define ATA_REG_LBA1      0x04            // LBA Mid Register
#define ATA_REG_LBA2      0x05            // LBA High Register
#define ATA_REG_HDDEVSEL  0x06            // Drive/Head Register
#define ATA_REG_COMMAND   0x07            // Command Register (write)
#define ATA_REG_STATUS    0x07            // Status Register (read)

// --- ATA Control Registers (biasanya untuk secondary channel) ---
#define ATA_REG_CONTROL   0x02          // Control Register (misalnya: Alternate Status atau Device Control)

// --- ATA Commands ---
#define ATA_CMD_READ_SECTORS      0x20  // Membaca sektor (PIO mode)
#define ATA_CMD_WRITE_SECTORS     0x30  // Menulis sektor (PIO mode)
#define ATA_CMD_IDENTIFY          0xEC  // Mengidentifikasi drive

// --- Drive Selection ---
#define ATA_MASTER 0xA0                 // Pilih drive master pada channel ATA
#define ATA_SLAVE  0xB0                 // Pilih drive slave pada channel ATA

// --- Standar Ukuran Sektor ---
#define ATA_SECTOR_SIZE 512

// --- Definisi Channel ATA ---
#define NUM_CHANNELS 2   // Contoh: Primary dan Secondary

// Declaration
void initATA();
int32_t read_ata_pio(ata_drive_t *drive, uint32_t lba, uint8_t sector_count, uint8_t *buffer);           // LBA start from 1
int32_t write_ata_pio(ata_drive_t *drive, uint32_t lba, uint8_t sector_count, uint8_t *buffer);

#endif                   // __ATA_H
