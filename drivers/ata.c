#include <drivers/ata.h>
#include <drivers/VGAColorSeq.h>
#include <ioport.h>
#include <stdio.h>
#include <string.h>

// Inisialisasi channel: Primary (0x1F0, 0x3F6) dan Secondary (0x170, 0x376)
ata_channel_t channels[NUM_CHANNELS] = { {0x1F0, 0x3F6, "Primary"}, {0x170, 0x376, "Secondary"} };
ata_drive_t ata_drives[MAX_ATA_DRIVES];
uint8_t ata_drive_count = 0;

/* Fungsi untuk mengirim perintah ATA IDENTIFY ke drive tertentu dan membaca datanya.
 * Jika drive terdeteksi, informasi drive (model dan total sektor) disimpan ke global array.
 * Parameter tambahan channel_index dan drive_index digunakan untuk menyimpan informasi
 * drive ke struktur yang tepat. */
void ata_identify_drive(ata_channel_t *channelPointer, uint8_t drive_sel) {
   // drive_sel: ATA_MASTER (0xA0) untuk Master, ATA_SLAVE (0xB0) untuk Slave
   // Pilih drive dengan menulis ke register HDDEVSEL
   outb(channelPointer->io_base + ATA_REG_HDDEVSEL, drive_sel);
   io_wait();

   // Bersihkan register-register yang diperlukan sebelum mengirim perintah IDENTIFY
   outb(channelPointer->io_base + ATA_REG_FEATURES, 0);
   outb(channelPointer->io_base + ATA_REG_SECCOUNT0, 0);
   outb(channelPointer->io_base + ATA_REG_LBA0, 0);
   outb(channelPointer->io_base + ATA_REG_LBA1, 0);
   outb(channelPointer->io_base + ATA_REG_LBA2, 0);

   // Kirim perintah IDENTIFY
   outb(channelPointer->io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
   io_wait();

   // Baca status untuk memastikan apakah drive ada
   uint8_t status = inb(channelPointer->io_base + ATA_REG_STATUS);
   if (status == 0)     // Tidak ada drive pada posisi ini
     return;

   // Tunggu hingga bit BSY (0x80) hilang
   while (status & 0x80)
     status = inb(channelPointer->io_base + ATA_REG_STATUS);

   // Jika terjadi error (bit ERR), drive mungkin tidak mendukung IDENTIFY
   if (status & 0x01) return;

   // Tunggu hingga bit DRQ (0x08) aktif, menandakan data siap dibaca
   while (!(status & 0x08))
     status = inb(channelPointer->io_base + ATA_REG_STATUS);

   // Baca 256 kata (512 byte) data IDENTIFY
   uint16_t buffer[256];
   for (int32_t i = 0; i < 256; i++) {
     buffer[i] = inw(channelPointer->io_base + ATA_REG_DATA);
   }

   // Ekstrak model drive dari kata 27 sampai 46 (20 kata, 40 karakter)
   char model[41];
   for (int32_t i = 0; i < 20; i++) {
     model[i * 2]     = buffer[27 + i] >> 8;
     model[i * 2 + 1] = buffer[27 + i] & 0xFF;
   }
   model[40] = '\0';
   // Hapus spasi ekstra dari kanan (opsional)
   for (int32_t i = 39; i >= 0; i--) {
     if (model[i] == ' ')
         model[i] = '\0';
     else
         break;
   }

   // Ekstrak total sektor (LBA28) dari kata 60 dan 61
   uint32_t total_sectors = ((uint32_t)buffer[61] << 16) | buffer[60];
   uint32_t logical_sector_size = 512;

   // Simpan informasi drive ke dalam struktur
   ata_drive_t drive;
   drive.channelPointer = channelPointer;
   drive.drive = drive_sel;  // 0 untuk Master, 1 untuk Slave
   strncpy(drive.model, model, 41);
   drive.total_sectors = total_sectors;

   // Mendapatkan Logical Sector Size
   if (buffer[106] & (1 << 10)) {
      logical_sector_size = (buffer[118] << 16) | buffer[117];
   }

   // Simpan ke array global
   ata_drives[ata_drive_count++] = drive;

   printf(CYAN "[ >> ] ATA Drive %s-%s " RESET YELLOW "(Model: %s, Sectors: %u, LSize: %uB)\n" RESET,
          drive.channelPointer->name,
          (drive.drive == ATA_MASTER) ? "Master" : "Slave",
          drive.model,
          drive.total_sectors,
          logical_sector_size);
}

/*
 * Fungsi inisialisasi ATA.
 * Melakukan iterasi pada setiap channel ATA dan posisi drive (Master/Slave),
 * memanggil fungsi IDENTIFY untuk masing-masing drive.
 */
void initATA() {
   // Reset jumlah drive yang terdeteksi
   ata_drive_count = 0;

   // printf("Scanning ATA/IDE Drive : \n");

   // Iterasi setiap channel ATA
   for (int32_t ch = 0; ch < NUM_CHANNELS; ch++) {
     // Iterasi untuk masing-masing drive: 0 untuk Master, 1 untuk Slave
     for (int32_t d = 0; d < NUM_DRIVES_PER_CHANNEL; d++) {
         uint8_t drive_sel = (d == 0) ? 0xA0 : 0xB0;
         // printf("Memeriksa %s channel, %s drive ...\n",
         //        channel.name, (d == 0) ? "Master" : "Slave");
         ata_identify_drive(&channels[ch], drive_sel);
     }
   }
}

/* Fungsi ATA read PIO dengan deteksi error.
 * Membaca 'sector_count' sektor dari drive ATA pada LBA tertentu.
 * Buffer harus memiliki ruang sebesar (sector_count * ATA_SECTOR_SIZE) byte.
 * Return 0 jika sukses, atau nilai negatif jika terjadi error. */
int32_t read_ata_pio(ata_drive_t *drive, uint32_t lba, uint8_t sector_count, uint8_t *buffer) {
    outb(drive->channelPointer->io_base + ATA_REG_HDDEVSEL, drive->drive | ((lba >> 24) & 0x0F));
    io_wait();

    // Bersihkan register fitur dan set jumlah sektor serta alamat LBA
    outb(drive->channelPointer->io_base  + ATA_REG_FEATURES, 0);
    outb(drive->channelPointer->io_base  + ATA_REG_SECCOUNT0, sector_count);
    outb(drive->channelPointer->io_base  + ATA_REG_LBA0, (uint8_t)lba);
    outb(drive->channelPointer->io_base  + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    outb(drive->channelPointer->io_base  + ATA_REG_LBA2, (uint8_t)(lba >> 16));

    // Kirim perintah baca sektor
    outb(drive->channelPointer->io_base  + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);
    io_wait();

    // Periksa status awal untuk memastikan drive ada
    uint8_t status = inb(drive->channelPointer->io_base  + ATA_REG_STATUS);
    if (status == 0) return -1;        // Drive tidak ada atau tidak merespon

    // Tunggu hingga BSY (0x80) hilang
    while (status & 0x80)  status = inb(drive->channelPointer->io_base  + ATA_REG_STATUS);

    // Jika terjadi error pada status (bit ERR aktif)
    if (status & 0x01)  return -2;

    // Proses pembacaan tiap sektor
    for (int32_t s = 0; s < sector_count; s++) {
        // Tunggu hingga data siap (DRQ aktif, bit 3)
        while (!((status = inb(drive->channelPointer->io_base  + ATA_REG_STATUS)) & 0x08))
            if (status & 0x01)   return -3;  // Deteksi error selama transfer

        // Baca 256 kata (512 byte) dari port data ATA
        for (int32_t i = 0; i < 256; i++) {
            uint16_t data = inw(drive->channelPointer->io_base  + ATA_REG_DATA);
            ((uint16_t*)buffer)[s * 256 + i] = data;
        }
    }

    return 0;
}

/* Fungsi ATA write PIO dengan deteksi error.
 * Menulis 'sector_count' sektor ke drive ATA pada LBA tertentu.
 * Buffer harus menyediakan (sector_count * ATA_SECTOR_SIZE) byte data.
 * Return 0 jika sukses, atau nilai negatif jika terjadi error. */
// int32_t write_ata_pio(uint16_t io_base, uint16_t ctrl_base, uint8_t drive_sel,
//                   uint32_t lba, uint8_t sector_count, uint8_t *buffer) {
int32_t write_ata_pio(ata_drive_t *drive, uint32_t lba, uint8_t sector_count, uint8_t *buffer) {
    // Pilih drive dan set LBA tinggi (bit 24-27)
    outb(drive->channelPointer->io_base + ATA_REG_HDDEVSEL, drive->drive | ((lba >> 24) & 0x0F));
    io_wait();

    // Bersihkan register fitur dan set jumlah sektor serta alamat LBA
    outb(drive->channelPointer->io_base + ATA_REG_FEATURES, 0);
    outb(drive->channelPointer->io_base + ATA_REG_SECCOUNT0, sector_count);
    outb(drive->channelPointer->io_base + ATA_REG_LBA0, (uint8_t)lba);
    outb(drive->channelPointer->io_base + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    outb(drive->channelPointer->io_base + ATA_REG_LBA2, (uint8_t)(lba >> 16));

    // Kirim perintah tulis sektor
    outb(drive->channelPointer->io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS);
    io_wait();

    // Periksa status awal untuk memastikan drive ada
    uint8_t status = inb(drive->channelPointer->io_base + ATA_REG_STATUS);
    if (status == 0) return -1;  // Drive tidak ada atau tidak merespon

    // Tunggu hingga BSY hilang
    while (status & 0x80)
        status = inb(drive->channelPointer->io_base + ATA_REG_STATUS);

    // Jika terjadi error pada status
    if (status & 0x01)
        return -2;

    // Proses penulisan tiap sektor
    for (int32_t s = 0; s < sector_count; s++) {
        // Tunggu hingga drive siap menerima data (DRQ aktif)
        while (!((status = inb(drive->channelPointer->io_base + ATA_REG_STATUS)) & 0x08)) {
            if (status & 0x01)
                return -3;
        }
        // Tulis 256 kata (512 byte) ke port data ATA
        for (int32_t i = 0; i < 256; i++) {
            uint16_t data = ((uint16_t*)buffer)[s * 256 + i];
            outw(drive->channelPointer->io_base + ATA_REG_DATA, data);
        }
    }

    // (Opsional) Lakukan flush cache dengan perintah ATA_CMD_CACHE_FLUSH jika diperlukan
    // outb(io_base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    // io_wait();

    return 0;
}
