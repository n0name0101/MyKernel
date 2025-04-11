#include <stdio.h>
#include <types.h>
#include <boolean.h>
#include <string.h>
#include <drivers/cpuid.h>
#include <drivers/pci.h>
#include <drivers/VGA.h>
#include <drivers/VGAColorSeq.h>
#include <drivers/ata.h>

#define EDD_ADDR (void *)0x7E00
#define KERNEL_ADDR (void *)0x10000

typedef struct {
    uint16_t buffer_size;         // 0x00 - Size of this structure (e.g. 0x1A or 0x42)
    uint16_t information_flags;   // 0x02 - Bit 0: CHS valid, Bit 1: LBA supported
    uint32_t cylinders;           // 0x04 - Number of cylinders
    uint32_t heads;               // 0x08 - Number of heads
    uint32_t sectors_per_track;  // 0x0C - Sectors per track
    uint64_t total_sectors;      // 0x10 - Total number of sectors (LBA addressing)
    uint16_t bytes_per_sector;   // 0x18 - Usually 512
    uint32_t dpte_ptr;           // 0x1A - EDD DPTE pointer (optional, may be 0)
    uint16_t key;                // 0x1E - Key (optional)
    char host_bus[4];            // 0x20 - Host bus type string ("PCI ", "USB", etc.)
    char interface_type[8];      // 0x24 - Interface type string ("ATA     ", "SATA    ", etc.)
    uint64_t interface_path;     // 0x2C - Device/interface path (optional)
    uint64_t device_path;        // 0x34 - BIOS device path (optional)
    uint8_t reserved[16];        // 0x3C - Reserved / future use
} __attribute__((packed)) edd_drive_params_t;            // This is to get BOOTED Drive Information

typedef void (*KernelMainFuncPtr)(uint32_t*, uint16_t, uint16_t);          // Kernel Declaration

// Declaration
void memcpy(void* dest, const void* src, uint32_t count);
void memset(void* dest, int32_t value, uint32_t count);

__attribute__((section(".endofprogram")))
uint32_t ENDMAGIC = 0x82825CFF;

extern uint32_t* vbe_physical_base_pointer;
extern uint16_t x_resolution;          // Width
extern uint16_t y_resolution;          // Height

extern ata_drive_t ata_drives[];
extern uint8_t ata_drive_count;

// Global flags untuk dukungan fitur CPU
bool g_has_sse2 = FALSE;

int32_t get_boot_drive_info() {
   edd_drive_params_t *edd = (edd_drive_params_t *)EDD_ADDR;

   // Basic validation
   if (edd->buffer_size < 0x1A || edd->total_sectors == 0) {
      printf(BG_RED WHITE BOLD "[ERR]" RESET RED " EDD info not available or unsupported.\n" RESET);
      return 0;
   }

   printf(CYAN "[ >> ] EDD Boot Drive Information:\n" RESET);

   // Printing Host Bus
   printf("  Host Bus         : " YELLOW "%c%c%c%c\n" RESET,
          edd->host_bus[0], edd->host_bus[1], edd->host_bus[2], edd->host_bus[3]);

   // Printing Interface Type
   printf("  Interface Type   : " YELLOW "%c%c%c%c%c%c%c%c\n" RESET,
          edd->interface_type[0], edd->interface_type[1], edd->interface_type[2], edd->interface_type[3],
          edd->interface_type[4], edd->interface_type[5], edd->interface_type[6], edd->interface_type[7]);

   // Informasi jumlah sektor dan ukuran sektor
   printf("  Total Sectors    : " YELLOW "%d\n" RESET, edd->total_sectors);
   printf("  Bytes Per Sector : " YELLOW "%d\n\n" RESET, edd->bytes_per_sector);

   // Deteksi tipe drive
   if (strncmp("PCI ATA", edd->interface_type, 7) == 0)
      return 1;
   else if(strncmp("PCI SATA", edd->interface_type, 8) == 0)
      return 2;
   else if(strncmp("PCI USB", edd->interface_type, 7) == 0)
      return 3;

   return 0;
}

void print_memory(void *addresspointer, uint32_t size, uint32_t row_size) {
    uint8_t *ptr = (uint8_t *) addresspointer;

    for (uint32_t offset = 0; offset < size; offset += row_size) {
        // Print address in bold cyan
        printf(BOLD CYAN "%.08x: " RESET, (uint32_t)(ptr + offset));

        // Print hex values in white
        for (int32_t j = 0; j < row_size; j++) {
            if (offset + j < size)
                printf(WHITE "%.02x " RESET, ptr[offset + j]);
            else
                printf("   ");
        }

        printf(" |");

        // Print ASCII representation
        for (int32_t j = 0; j < row_size; j++) {
            if (offset + j < size) {
                unsigned char c = ptr[offset + j];
                if (my_isprint(c))
                    printf(GREEN "%c" RESET, c);
                else
                    printf(BRIGHT_BLACK "." RESET);
            } else {
                printf(" ");
            }
        }
        printf("|\n");
    }
}

/* Fungsi utama loader (dijalankan di protected mode 32-bit) */
void initMain(void) {
   initVGA();

   if (ENDMAGIC != 0x82825CFF) {
      printf(BG_RED WHITE BOLD "[ERR]" RESET RED " INIT PROGRAM CORRUPT\n" RESET);
      return;
   }

   printf(YELLOW "[ .. ]" RESET WHITE " Loading Kernel ...\n" RESET);
   switch(get_boot_drive_info()) {
      case 1:  // ATA
         printf(YELLOW "[ .. ]" RESET WHITE " Load Kernel From ATA ...\n" RESET);
         initATA();
         for (uint8_t i = 0; i < ata_drive_count; i++) {
            if (read_ata_pio(&ata_drives[i], 1, 1, KERNEL_ADDR) == 0) {
               if (*(uint16_t *)(((char *)KERNEL_ADDR)+510) == 0xAA55 &&
                   ((edd_drive_params_t *)EDD_ADDR)->total_sectors == ata_drives[i].total_sectors &&
                   (read_ata_pio(&ata_drives[i], 57, 100, KERNEL_ADDR) == 0)) {

                   printf(GREEN BOLD "[ OK ]" RESET WHITE " Kernel Loaded Successfully\n" RESET);
                   ((KernelMainFuncPtr)KERNEL_ADDR)(vbe_physical_base_pointer, x_resolution, y_resolution);
               }
            }
         }
         break;

      default:
         printf(BG_RED WHITE BOLD "[ERR]" RESET RED " Load Kernel Failed: Driver Not Found\n" RESET);
         break;
   }

   printf(BG_RED WHITE BOLD "[ERR]" RESET RED " Boot Drive Not Found ...\n" RESET);

   pci_scan_bus();

   //printf(CYAN "[ >> ] Memory Checking:\n" RESET);
   //print_memory(EDD_ADDR, 512, 16);

   while(1);
}
