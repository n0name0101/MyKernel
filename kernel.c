#include <stdio.h>
#include <types.h>
#include <boolean.h>
#include <string.h>
#include <drivers/cpuid.h>
#include <drivers/pci.h>
#include <drivers/VGA.h>
#include <drivers/VGAColorSeq.h>
#include <drivers/ata.h>
#include <drivers/8259PIC.h>
#include <drivers/TimerPIT.h>

typedef struct {
    uint16_t buffer_size;           // 0x00 - Size of this structure (e.g. 0x1A or 0x42)
    uint16_t information_flags;     // 0x02 - Bit 0: CHS valid, Bit 1: LBA supported
    uint32_t cylinders;             // 0x04 - Number of cylinders
    uint32_t heads;                 // 0x08 - Number of heads
    uint32_t sectors_per_track;     // 0x0C - Sectors per track
    uint64_t total_sectors;         // 0x10 - Total number of sectors (LBA addressing)
    uint16_t bytes_per_sector;      // 0x18 - Usually 512
    uint32_t dpte_ptr;              // 0x1A - EDD DPTE pointer (optional, may be 0)
    uint16_t key;                   // 0x1E - Key (optional)
    char host_bus[4];               // 0x20 - Host bus type string ("PCI ", "USB", etc.)
    char interface_type[8];         // 0x24 - Interface type string ("ATA     ", "SATA    ", etc.)
    uint64_t interface_path;        // 0x2C - Device/interface path (optional)
    uint64_t device_path;           // 0x34 - BIOS device path (optional)
    uint8_t reserved[16];           // 0x3C - Reserved / future use
} __attribute__((packed)) edd_drive_params_t;            // This is to get BOOTED Drive Information From BIOS

// Declaration
void memcpy(void* dest, const void* src, uint32_t count);
void memset(void* dest, int32_t value, uint32_t count);

extern ata_channel_t channels[];
extern ata_drive_t ata_drives[];
extern uint8_t ata_drive_count;

extern bool g_has_sse2;
extern bool g_has_avx2;

int32_t get_boot_drive_info();
void print_memory(void *addresspointer, uint32_t size, uint32_t row_size);

// VGA information
uint16_t x_resolution = 0;
uint16_t y_resolution = 0;
uint32_t *vbe_physical_base_pointer = NULL;

/* Fungsi utama loader (dijalankan di protected mode 32-bit) */
void kernelMain(uint32_t* _vbe_physical_base_pointer, uint16_t _x_resolution, uint16_t _y_resolution) {
   // Get Screen Resolution, and FrameBuffer Address From Init
   x_resolution = _x_resolution;
   y_resolution = _y_resolution;
   vbe_physical_base_pointer = _vbe_physical_base_pointer;

   initVGA();
   printf(CYAN "[ >> ] Initializing Kernel Environment...\n" RESET);
   printf(MAGENTA "[ !! ] Welcome to MyKernel\n" RESET);

   // Deteksi fitur CPU saat startup
   detect_cpu_features();  // Detect SSE2 IF Available then Enable it

   printf(YELLOW "[ .. ] CPU Support SSE2 Instructions ? : " RESET "%s\n", g_has_sse2 ? GREEN BOLD "Yes" RESET : RED "No" RESET);
   printf(YELLOW "[ .. ] CPU Support AVX2 Instructions ? : " RESET "%s\n", g_has_avx2 ? GREEN BOLD "Yes" RESET : RED "No" RESET);

   // PCI Scanning
   printf(CYAN "[ >> ] Scanning PCI Bus...\n" RESET);
   pci_scan_bus();

   // Setup Interrupt 8259PIC
   int pic_status = init8259PIC(0x08);  // 0x08 is Global Descriptor Table Code Segment
   if (pic_status == 0)
      printf(GREEN BOLD "[ OK ]" RESET " PIC Initialized Successfully\n");
   else
      printf(BG_RED WHITE BOLD "[ERR]" RESET RED " PIC Initialization Failed\n" RESET);

   // PIT Initialization
   printf(GREEN BOLD "[ OK ]" RESET " PIT Timer Initialized\n");
   initPIT(1000);  // Interrupt 1000 kali dalam 1 Detik

   // Cursor timer
   vga_set_cursor_timer(NULL);

   printf(GREEN BOLD "[ OK ]" RESET " Interrupt Flag Set\n");
   interruptActive();  // Enable Interrupts

   // ATA Init
   printf(CYAN "[ >> ] Initializing ATA Devices...\n" RESET);
   initATA();

   // Jika ingin testing read/write drive bisa aktifkan ini:
   /*
   char *addresspointer = (char *) 0x00010000;

   printf(CYAN "[ >> ] READ FROM DRIVE : " RESET);
   if(read_ata_pio(&ata_drives[0], 1, 1, addresspointer) == 0)
      printf(GREEN BOLD "[ OK ]" RESET "\n");
   else
      printf(BG_RED WHITE BOLD "[ERR]" RESET RED " FAILED\n" RESET);

   printf(CYAN "[ >> ] WRITE TO DRIVE : " RESET);
   if(write_ata_pio(&ata_drives[0], 35, 1, addresspointer) == 0)
      printf(GREEN BOLD "[ OK ]" RESET "\n");
   else
      printf(BG_RED WHITE BOLD "[ERR]" RESET RED " FAILED\n" RESET);

   print_memory(addresspointer, 512, 16);
   */

   while (1);
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
