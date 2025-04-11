#include <drivers/ahci.h>
#include <ioport.h>
#include <stdio.h>
#include <string.h>
#include <utils.h>

// --- Fungsi Helper: Start/Stop Command Engine pada Port ---
static void start_cmd(HBA_PORT_t *port) {
    // Jika sebelumnya port telah di-stop, pastikan CR (Command List Running) sudah false
    while (port->cmd & (1 << 15)); // tunggu jika CR (bit 15) aktif
    // Set FRE (FIS Receive Enable) dan ST (Start)
    port->cmd |= (1 << 4);  // FRE
    port->cmd |= (1 << 0);  // ST
}

static void stop_cmd(HBA_PORT_t *port) {
    // Clear ST untuk menghentikan engine
    port->cmd &= ~(1 << 0); // Clear ST
    // Tunggu hingga CR (Command Running) hilang
    while (port->cmd & (1 << 15));
    // Clear FRE setelah engine berhenti
    port->cmd &= ~(1 << 4);
}

// --- AHCI Initialization ---
// Fungsi inisialisasi AHCI pada ABAR (AHCI Base Memory Region).
// Biasanya, ABAR didapat dari konfigurasi PCI.
void ahci_init(HBA_MEM_t *abar) {
    // Aktifkan AHCI dengan meng-set GHC.AE (bit 31)
    abar->ghc |= (1 << 31);
    // Membaca bit Ports Implemented (PI) untuk enumerasi port
    uint32_t pi = abar->pi;
    printf("[AHCI] Ports Implemented: 0x%08X\n", pi);

    // Port array mulai dari offset 0x100 di ABAR
    HBA_PORT_t *port_array = (HBA_PORT_t *)((uint8_t *)abar + 0x100);

    for (int i = 0; i < AHCI_MAX_PORTS; i++) {
        if (pi & (1 << i)) {
            HBA_PORT_t *port = &port_array[i];
            // Periksa status port
            uint32_t ssts = port->ssts;
            uint8_t det = ssts & 0x0F;         // Device detection
            uint8_t ipm = (ssts >> 8) & 0x0F;    // Interface power management status
            // Cek apakah ada device yang terpasang dan aktif (contoh: det==3 dan ipm==1)
            if (det != 3 || ipm != 1) {
                continue;
            }
            // Tampilkan signature untuk referensi (misalnya, ATA drive biasanya 0x00000101)
            printf("[AHCI] Port %d: Device Detected, Signature: 0x%08X\n", i, port->sig);

            // Untuk demo, kita asumsikan memory untuk Command List dan FIS sudah dialokasikan secara statis.
            // Misalnya, kita mengalokasikan memory 1K-aligned untuk command list dan 256-byte-aligned untuk FIS.
            // Di lingkungan nyata, sebaiknya gunakan alokasi memori kernel dan mapping MMIO.
            static uint8_t cmd_list_buf[32][1024] __attribute__((aligned(1024)));
            static uint8_t fis_buf[32][256] __attribute__((aligned(256)));

            // Set Command List Base Address
            port->clb = (uint32_t)(uint32_t *)cmd_list_buf[i];
            port->clbu = 0;
            // Set FIS Base Address
            port->fb = (uint32_t)(uint32_t *)fis_buf[i];
            port->fbu = 0;

            // Stop port command engine sebelum konfigurasi ulang
            stop_cmd(port);

            // Bersihkan registers yang diperlukan
            port->is = (uint32_t)-1;  // Clear semua status interupsi
            port->ci = 0;

            // Mulai port command engine
            start_cmd(port);
        }
    }
}

// --- Fungsi AHCI Read ---
// Fungsi berikut menggunakan pendekatan sederhana untuk menyiapkan perintah baca menggunakan satu entry.
// Kode ini merupakan skeleton dan hanya digunakan untuk pendidikan.
int ahci_read(HBA_PORT_t *port, uint64_t lba, uint32_t sector_count, uint8_t *buffer) {
    // Temukan slot perintah bebas pada port. Slot digunakan untuk menempatkan command.
    uint32_t slots = port->sact | port->ci;
    int slot = -1;
    for (int i = 0; i < 32; i++) {
        if ((slots & (1 << i)) == 0) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        printf("[AHCI Read] Tidak ada slot perintah yang bebas.\n");
        return -1;
    }

    // Ambil pointer ke Command List untuk slot yang dipilih.
    HBA_CMD_HEADER_t *cmdheader = (HBA_CMD_HEADER_t *)(uint32_t *)port->clb;
    // Bersihkan entry command header untuk slot ini
    memset(&cmdheader[slot], 0, sizeof(HBA_CMD_HEADER_t));
    // Untuk operasi baca, flag diatur untuk PIO data-in (0x25) dan panjang PRDT minimal
    cmdheader[slot].flags = (sizeof(HBA_CMD_TBL_t) - 1) | (5 << 16);  // Contoh nilai, sesuaikan dengan spesifikasi
    // Set PRDT length (misalnya, 1 untuk contoh sederhana)
    cmdheader[slot].prdtl = 1;

    // Dapatkan pointer ke Command Table (harus dialokasikan secara benar)
    HBA_CMD_TBL_t *cmdtbl = (HBA_CMD_TBL_t *)(uint32_t *)(cmdheader[slot].ctba);
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL_t));

    // Siapkan Command FIS (CFIS)
    uint8_t *cfis = cmdtbl->cfis;
    memset(cfis, 0, 64);
    cfis[0] = 0x27;              // Type: Host to Device FIS
    cfis[1] = 0x80;              // Set C bit untuk Command FIS
    cfis[2] = ATA_CMD_READ_SECTORS;  // Perintah ATA: Read Sectors
    // Isi LBA (48-bit) dan sector_count sesuai dengan spesifikasi FIS ATA.
    cfis[3] = (uint8_t)lba;
    cfis[4] = (uint8_t)(lba >> 8);
    cfis[5] = (uint8_t)(lba >> 16);
    cfis[6] = (uint8_t)((lba >> 24) & 0xFF);
    cfis[7] = (uint8_t)(lba >> 32);
    cfis[8] = (uint8_t)(lba >> 40);
    cfis[9] = (uint8_t)sector_count;

    // Setup PRDT (Physical Region Descriptor Table) di command table
    // Di sini, kita anggap buffer sudah dialokasikan dan berukuran minimal sector_count * AHCI_SECTOR_SIZE.
    // Untuk contoh, kita tidak membuat struktur PRDT lengkap, tetapi di implementasi nyata PRDT harus disiapkan.
    // (Biasanya, PRDT entry berisi address, byte count, dan flag interrupt.)

    // Bersihkan status port dan set perintah ke issue register
    port->is = (uint32_t)-1; // Bersihkan status interupsi
    port->ci = 1 << slot;    // Issue perintah pada slot yang dipilih

    // Tunggu hingga perintah selesai. (Polling, timeout sebaiknya diimplementasikan.)
    while (port->ci & (1 << slot)) {
        // Jika terjadi error (misalnya, CFS error), return error.
        if (port->is & (1 << 30)) {   // Contoh: bit kesalahan
            printf("[AHCI Read] Error terdeteksi pada port, status: 0x%08X\n", port->is);
            return -1;
        }
    }

    // Data telah ditransfer ke buffer oleh mekanisme DMA.
    return 0;
}

// --- Fungsi AHCI Write ---
// Mirip dengan fungsi baca, tetapi menggunakan perintah ATA_CMD_WRITE_SECTORS.
int ahci_write(HBA_PORT_t *port, uint64_t lba, uint32_t sector_count, uint8_t *buffer) {
    uint32_t slots = port->sact | port->ci;
    int slot = -1;
    for (int i = 0; i < 32; i++) {
        if ((slots & (1 << i)) == 0) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        printf("[AHCI Write] Tidak ada slot perintah yang bebas.\n");
        return -1;
    }

    HBA_CMD_HEADER_t *cmdheader = (HBA_CMD_HEADER_t *)(uint32_t *)port->clb;
    memset(&cmdheader[slot], 0, sizeof(HBA_CMD_HEADER_t));
    cmdheader[slot].flags = (sizeof(HBA_CMD_TBL_t) - 1) | (5 << 16);
    cmdheader[slot].prdtl = 1;

    HBA_CMD_TBL_t *cmdtbl = (HBA_CMD_TBL_t *)(uint32_t *)(cmdheader[slot].ctba);
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL_t));

    uint8_t *cfis = cmdtbl->cfis;
    memset(cfis, 0, 64);
    cfis[0] = 0x27;
    cfis[1] = 0x80;
    cfis[2] = ATA_CMD_WRITE_SECTORS;
    cfis[3] = (uint8_t)lba;
    cfis[4] = (uint8_t)(lba >> 8);
    cfis[5] = (uint8_t)(lba >> 16);
    cfis[6] = (uint8_t)((lba >> 24) & 0xFF);
    cfis[7] = (uint8_t)(lba >> 32);
    cfis[8] = (uint8_t)(lba >> 40);
    cfis[9] = (uint8_t)sector_count;

    port->is = (uint32_t)-1;
    port->ci = 1 << slot;

    while (port->ci & (1 << slot)) {
        if (port->is & (1 << 30)) {
            printf("[AHCI Write] Error terdeteksi pada port, status: 0x%08X\n", port->is);
            return -1;
        }
    }
    return 0;
}
