#include <drivers/usbxhci.h>
#include <drivers/VGAColorSeq.h>
#include <types.h>
#include <boolean.h>

void xhci_print_capability_info(xhci_cap_regs_t* cap_regs_pointer) {
    // Register awal (offset dari mmio_base)
    uint8_t cap_length    = cap_regs_pointer->caplength;
    uint16_t hci_version  = cap_regs_pointer->hci_version;
    uint32_t hcs_params1  = cap_regs_pointer->hcsparams1;
    uint32_t hcs_params2  = cap_regs_pointer->hcsparams2;
    uint32_t hcs_params3  = cap_regs_pointer->hcsparams3;
    uint32_t hcc_params1  = cap_regs_pointer->hccparams1;
    uint32_t dboff        = cap_regs_pointer->dboff;
    uint32_t rtsoff       = cap_regs_pointer->rtsoff;

    printf(YELLOW "[ .. ]" RESET WHITE " === xHCI Capability Registers ===" RESET "\n");

    printf(YELLOW "[ .. ]" RESET WHITE " CAPLENGTH      : 0x%.02x (%d bytes)" RESET "\n", cap_length, cap_length);
    printf(YELLOW "[ .. ]" RESET WHITE " HCIVERSION     : 0x%.04x (USB %d.%d)" RESET "\n", hci_version,
           (hci_version >> 8), (hci_version & 0xFF));
    printf(YELLOW "[ .. ]" RESET WHITE " HCSPARAMS1     : 0x%.08x" RESET "\n", hcs_params1);
    printf(YELLOW "[ .. ]" RESET WHITE "   - Max Slots       : %d" RESET "\n", (hcs_params1 & 0xFF));
    printf(YELLOW "[ .. ]" RESET WHITE "   - Max Interrupters: %d" RESET "\n", (hcs_params1 >> 8) & 0x3FF);
    printf(YELLOW "[ .. ]" RESET WHITE "   - Max Ports       : %d" RESET "\n", (hcs_params1 >> 24) & 0xFF);

    printf(YELLOW "[ .. ]" RESET WHITE " HCSPARAMS2     : 0x%.08x" RESET "\n", hcs_params2);
    printf(YELLOW "[ .. ]" RESET WHITE " HCSPARAMS3     : 0x%.08x" RESET "\n", hcs_params3);
    printf(YELLOW "[ .. ]" RESET WHITE " HCCPARAMS1     : 0x%.08x" RESET "\n", hcc_params1);
    printf(YELLOW "[ .. ]" RESET WHITE " DBOFF (Doorbell Offset) : 0x%.08x (at MMIO + 0x%x)" RESET "\n", dboff, dboff);
    printf(YELLOW "[ .. ]" RESET WHITE " RTSOFF (Runtime Offset) : 0x%.08x (at MMIO + 0x%x)" RESET "\n", rtsoff, rtsoff);
    printf(YELLOW "[ .. ]" RESET WHITE " Operational Registers at : MMIO + 0x%x" RESET "\n", cap_length);
    printf(YELLOW "[ .. ]" RESET WHITE " =================================" RESET "\n");

}

bool xhci_is_owned_by_bios(xhci_cap_regs_t *cap_regs_base) {
    uint32_t hccparams1 = cap_regs_base->hccparams1;
    uint32_t ext_cap_offset = (hccparams1 >> 16) & 0xFFFF;

    while (ext_cap_offset) {
        volatile uint32_t *cap_ptr = (volatile uint32_t *)((uint32_t *)cap_regs_base + ext_cap_offset);
        uint32_t cap = xhci_read32(cap_ptr);

        uint8_t cap_id = cap & 0xFF;
        if (cap_id == XHCI_EXT_CAP_ID_LEGACY_SUPPORT) {
            uint32_t bios_own = xhci_read32(cap_ptr + 1);  // USB Legacy Support register
            PRINTF_DEBUG("USB Legacy Support Register : %b", bios_own);
            return (bios_own & 0x00010001) == 0x00010001;  // BIOS_OWNED_SEMAPHORE and OS_OWNED_SEMAPHORE
        }

        ext_cap_offset = (cap >> 8) & 0xFFFC;
    }

    return FALSE;
}

// Fungsi untuk reset controller melalui register USBCMD di area operasional xHCI.
// Asumsinya, register operasional dimulai pada offset CAPLENGTH dari base.
void xhci_reset_controller(xhci_cap_regs_t *cap_regs_base) {
   volatile uint32_t *usbcmd = (volatile uint32_t *)((uint32_t *)cap_regs_base + cap_regs_base->caplength + USBCMD_OFFSET);
   uint32_t cmd;

   PRINTF_DEBUG("Melakukan reset xHCI controller usbcmd = %x...", usbcmd);

   // Aktifkan bit reset dengan menulis bit HCRST ke register USBCMD.
   cmd = xhci_read32(usbcmd);
   cmd |= USBCMD_HCRST;
   xhci_write32(usbcmd, cmd);

   // Tunggu sampai bit reset ter-clear, dengan timeout sederhana.
   int timeout = 1000;
   while ((xhci_read32(usbcmd) & USBCMD_HCRST) && timeout--) {
      // Delay loop sederhana, bisa diganti dengan fungsi delay sesuai platform.
      for (volatile int i = 0; i < 1000; i++);
   }

   if (timeout <= 0)
      printf(BG_RED WHITE BOLD "[ERR]" RESET RED " Timeout: Reset xHCI controller gagal." RESET);
   else
      printf(GREEN BOLD "[ OK ]" RESET WHITE " Reset xHCI controller berhasil.\n" RESET);
}
