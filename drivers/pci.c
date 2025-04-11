#include <drivers/pci.h>
#include <drivers/VGAColorSeq.h>
#include <drivers/usbxhci.h>
#include <utils.h>
#include <boolean.h>
#include <stdio.h>
#include <string.h>
#include <ioport.h>

// Fungsi membaca 32-bit di PCI configuration space
static inline uint32_t pci_config_read_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (1U << 31)
                     | ((uint32_t)bus << 16)
                     | ((uint32_t)device << 11)
                     | ((uint32_t)function << 8)
                     | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

// Fungsi membaca 8-bit dari PCI config space
static inline uint8_t pci_config_read_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t dword = pci_config_read_dword(bus, device, function, offset & 0xFC);
    uint8_t ret = (dword >> (8 * (offset & 3))) & 0xFF;
    return ret;
}

// Bantu: Akses PCI config space
uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = PCI_ENABLE_BIT | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS , address);
    // _asm_ volatile ("outl %0, %%dx" : : "a"(address), "d"(PCI_CONFIG_ADDRESS));

    // uint32_t value;
    // _asm_ volatile ("inl %%dx, %0" : "=a"(value) : "d"(PCI_CONFIG_DATA));
    // return value;
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data) {
    uint32_t old = pci_config_read32(bus, slot, func, offset & 0xFC);
    uint32_t shift = (offset & 2) * 8;
    old &= ~(0xFFFF << shift);
    old |= (data << shift);

    uint32_t address = PCI_ENABLE_BIT | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);

    outl(PCI_CONFIG_ADDRESS , address);
    outl(PCI_CONFIG_DATA , old);
    // _asm_ volatile ("outl %0, %%dx" : : "a"(address), "d"(PCI_CONFIG_ADDRESS));
    // _asm_ volatile ("outl %0, %%dx" : : "a"(old), "d"(PCI_CONFIG_DATA));
}

// Fungsi membaca 16-bit dari PCI config space
static inline uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t dword = pci_config_read_dword(bus, device, function, offset & 0xFC);
    uint16_t ret;
    // Jika offset tidak selaras 32-bit, ambil word yang benar
    ret = (dword >> (8 * (offset & 2))) & 0xFFFF;
    return ret;
}

// Fungsi untuk membaca Base Address Registers (BAR)
// BAR terletak di offset 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24
static void pci_get_bars(pci_device_t *dev) {
    for (int i = 0; i < 6; i++) {
        dev->bar[i] = pci_config_read_dword(dev->bus, dev->device, dev->function, 0x10 + i * 4);
    }
}

// Fungsi untuk memeriksa apakah fungsi PCI ada (vendor_id != 0xFFFF)
static int pci_device_exists(uint8_t bus, uint8_t device, uint8_t function) {
    uint16_t vendor = pci_config_read_word(bus, device, function, 0x00);
    return (vendor != 0xFFFF);
}

// Fungsi menampilkan informasi perangkat PCI
static void pci_print_device_info(const pci_device_t *dev) {
    printf(CYAN "[ >> ] PCI %d:%d.%d " RESET YELLOW "(VID: 0x%04x, DID: 0x%04x)\n" RESET,
           dev->bus, dev->device, dev->function, dev->vendor_id, dev->device_id);
    printf("        " YELLOW "Class: 0x%02x, Subclass: 0x%02x, IF: 0x%02x\n" RESET,
           dev->class_code, dev->subclass, dev->prog_if);

    bool has_bar = FALSE;
    printf("        BARs  :");
    for (int i = 0; i < 6; i++) {
        if (dev->bar[i] != 0) {
            printf(" BAR%d=" YELLOW "0x%08x" RESET, i, dev->bar[i]);
            has_bar = TRUE;
        }
    }
    if (!has_bar) printf(" " YELLOW "(none)" RESET);
    printf("\n\n");
}


// Fungsi untuk mengecek apakah perangkat PCI adalah AHCI (SATA) controller
// AHCI controller biasanya memiliki Base Class 0x01 (Mass Storage), Subclass 0x06 (SATA), Prog IF 0x01
static int is_ahci_controller(const pci_device_t *dev) {
    return (dev->class_code == 0x01) && (dev->subclass == 0x06) && (dev->prog_if == 0x01);
}

// Fungsi untuk mengecek apakah perangkat PCI adalah USB controller.
// USB controller umumnya memiliki Base Class 0x0C, Subclass 0x03.
// Prog IF berbeda-beda:
//   - UHCI: 0x00
//   - OHCI: 0x10
//   - EHCI: 0x20
//   - XHCI: 0x30 (umumnya menggunakan class code 0x0C, subclass 0x03 dan prog IF 0x30).
static int is_usb_controller(const pci_device_t *dev) {
    return (dev->class_code == 0x0C) && (dev->subclass == 0x03);
}

/// Fungsi utama untuk enumerasi PCI
// Membaca bus 0-255, device 0-31 dan function 0-7
void pci_scan_bus(void) {
   pci_device_t dev;
   memset(&dev, 0, sizeof(dev));

   printf(CYAN "[ >> ] PCI Bus Scanning Started...\n" RESET);

   for (uint16_t bus = 0; bus < 256; bus++) {
      for (uint8_t device = 0; device < 32; device++)
         for (uint8_t function = 0; function < 8; function++) {
            if (!pci_device_exists(bus, device, function))
               continue;

            dev.bus = bus;
            dev.device = device;
            dev.function = function;
            dev.vendor_id = pci_config_read_word(bus, device, function, 0x00);
            dev.device_id = pci_config_read_word(bus, device, function, 0x02);
            dev.prog_if   = pci_config_read_byte(bus, device, function, 0x09);
            dev.subclass  = pci_config_read_byte(bus, device, function, 0x0A);
            dev.class_code= pci_config_read_byte(bus, device, function, 0x0B);
            pci_get_bars(&dev);

            // Jika perangkat adalah AHCI
            if (is_ahci_controller(&dev)) {
               printf(MAGENTA "[ !! ] AHCI SATA Controller finded\n" RESET);
               printf("        ABAR (BAR5): " YELLOW "0x%08x\n" RESET, dev.bar[5]);
            }

            // Jika perangkat adalah USB controller
            if (is_usb_controller(&dev))
               switch (dev.prog_if){
                  case 0x00 :              //UHCI DRIVER
                     printf(MAGENTA "[ !! ] UHCI USB Controller finded\n" RESET);
                     break;
                  case 0x30 :              //XHCI DRIVER
                     printf(MAGENTA "[ !! ] XHCI USB Controller finded\n" RESET);
                     for (int i = 0; i < 6; i++) {
                        if (dev.bar[i] != 0){
                           if (dev.bar[i] & 0x1) continue; // Bukan MMIO

                           uint32_t mmio_base = dev.bar[i] & 0xFFFFFFF0;

                           // Akses register xHCI (tanpa paging, flat memory)
                           xhci_cap_regs_t *xhci_regs = (xhci_cap_regs_t *)mmio_base;

                           // Aktifkan memory access dan bus mastering
                           uint16_t command = pci_config_read_word(bus, device, function, 0x04);
                           PRINTF_DEBUG("Command Before : %.16b", command);
                           PRINTF_DEBUG("XHCI CAPLENGTH = %02x\n", xhci_regs->caplength);

                           command |= (1 << 1); // Memory Space Enable
                           command |= (1 << 2); // Bus Master Enable
                           pci_config_write16(bus, device, function, 0x04, command);

                           PRINTF_DEBUG("Command After  : %.16b", pci_config_read_word(bus, device, function, 0x04));
                           PRINTF_DEBUG("XHCI CAPLENGTH = %02x\n", xhci_regs->caplength);

                           xhci_print_capability_info(xhci_regs);
                           init_xhci_driver(xhci_regs) ;
                           break; // Sudah ketemu, keluar
                        }
                     }
                     break;
                  default   :
                     printf(BG_RED WHITE BOLD "[ERR]" RESET RED " USB Driver Not Found ...\n" RESET);
                     break;
               }
         }
   }

   printf(GREEN BOLD "[ OK ]" RESET " PCI Bus Scanning Completed.\n");
}
