#include <types.h>
#include <stdio.h>

// Definisi ukuran ring dan alignment
#define TRB_RING_SIZE   256
#define ALIGN_64        __attribute__((aligned(64)))

// Definisi bit register USBCMD (sesuai xHCI spec)
#define USBCMD_RUN      (1 << 0)  // Run/Stop bit: 1 = Run
#define USBCMD_HCRST    (1 << 1)  // Host Controller Reset

// Definisi offset register dari MMIO (berdasarkan xHCI spec)
#define CAPLENGTH_OFFSET    0x00    // Offset untuk CAPLENGTH (1 byte) di ruang kemampuan
// Operational registers start pada (mmio_base + CAPLENGTH)
#define USBCMD_OFFSET       0x00    // Operational: Command register (USBCMD)
#define USBSTS_OFFSET       0x04    // Operational: Status register (USBSTS)
#define CRCR_OFFSET         0x18    // Operational: Command Ring Control Register (CRCR)
#define DBOFF_OFFSET        0x2000    // Operational: Doorbell registers mulai dari offset 0x20

// Struktur TRB (Transfer Request Block) 16 byte, sesuai spesifikasi xHCI
typedef struct __attribute__((packed)) {
    uint32_t parameter_low;    // Parameter 32-bit lower
    uint32_t parameter_high;   // Parameter 32-bit high (jika diperlukan)
    uint32_t status;           // Status dan informasi panjang data
    uint32_t control;          // Field kontrol termasuk tipe TRB dan cycle bit
} xhci_trb_t;

// Definisi tipe TRB, misal: NOOP
#define TRB_TYPE_NOOP   0x09

// Fungsi helper untuk menyusun field control TRB:
// Bits 10-15: tipe TRB, bit 0: cycle, sisanya flag tambahan
static inline uint32_t make_trb_control(uint8_t type, uint32_t flags) {
    return ((uint32_t)type << 10) | flags;
}

// Alokasi statis untuk Command Ring dan Event Ring (tanpa malloc)
static xhci_trb_t cmd_ring[TRB_RING_SIZE] ALIGN_64;
static xhci_trb_t evt_ring[TRB_RING_SIZE] ALIGN_64;

// Variabel global untuk indeks ring dan cycle bit
static volatile uint32_t cmd_ring_enqueue = 0;
static volatile uint32_t evt_ring_dequeue = 0;
static volatile uint32_t cmd_cycle = 1; // cycle bit awal untuk command ring
static volatile uint32_t evt_cycle = 1; // cycle bit awal untuk event ring

// Memory barrier sederhana untuk memastikan urutan penulisan memori
static inline void memory_barrier(void) {
    __asm__ volatile ("" ::: "memory");
}

// Fungsi untuk menulis ke register xHCI (operasional)
// 'base' adalah base address dari operational registers (mmio_base + CAPLENGTH)
static inline void xhci_write_reg(uint32_t base, uint32_t offset, uint32_t value) {
    volatile uint32_t* reg = (volatile uint32_t*)(base + offset);
    *reg = value;
}

// Fungsi untuk membaca register xHCI
static inline uint32_t xhci_read_reg(uint32_t base, uint32_t offset) {
    volatile uint32_t* reg = (volatile uint32_t*)(base + offset);
    return *reg;
}

// Fungsi untuk memasukkan (enqueue) TRB ke command ring
static void enqueue_cmd_trb(const xhci_trb_t *trb) {
    // Salin TRB dan set cycle bit sesuai
    xhci_trb_t new_trb = *trb;
    new_trb.control |= (cmd_cycle & 1);

    cmd_ring[cmd_ring_enqueue] = new_trb;
    memory_barrier();  // Pastikan data telah ditulis sebelum update pointer

    cmd_ring_enqueue++;
    if (cmd_ring_enqueue >= TRB_RING_SIZE) {
        // Jika mencapai akhir ring, reset pointer dan flip cycle bit
        cmd_ring_enqueue = 0;
        cmd_cycle ^= 1;
    }
}

// Fungsi untuk mengatur CRCR (Command Ring Control Register)
// Pada sistem 32-bit, alamat command ring dapat dikirim secara langsung (pastikan 16-byte aligned)
// Catatan: Konversi alamat virtual-ke-fisik mungkin diperlukan untuk implementasi nyata.
static void xhci_set_crcr(uint32_t op_regs_base, uint32_t cmd_ring_phys_addr) {
    // Format CRCR: Bits 4..31: alamat command ring, Bit 0: cycle (set 1)
    uint32_t crcr_value = (cmd_ring_phys_addr & ~0xF) | 1;  // set cycle bit
    xhci_write_reg(op_regs_base, CRCR_OFFSET, crcr_value);
}

// Fungsi untuk "ring doorbell" yang memberi tahu controller bahwa ada perintah baru
static void xhci_ring_doorbell(uint32_t op_regs_base, uint32_t doorbell_index) {
    // Setiap doorbell memiliki offset 4 byte (misalnya, index 0 untuk command ring)
    uint32_t doorbell_offset = DBOFF_OFFSET + (doorbell_index * 4);
    xhci_write_reg(op_regs_base, doorbell_offset, 0);
}

// Fungsi inisialisasi xHCI driver yang mencakup reset, running, dan inisialisasi ring.
// Parameter: mmio_base_address yang disediakan (misalnya, dari PCI BAR)
int init_xhci_driver(uint32_t mmio_base_address) {
    // 1. Baca CAPLENGTH dari area capability
    uint8_t cap_length = *((volatile uint8_t*)(mmio_base_address + CAPLENGTH_OFFSET));
    // Tentukan base address untuk operational registers
    uint32_t op_regs_base = mmio_base_address + cap_length;

    // 2. Reset Host Controller                 // WORK
    uint32_t usbcmd = xhci_read_reg(op_regs_base, USBCMD_OFFSET);
    // Set bit HCRST untuk memulai reset
    usbcmd |= USBCMD_HCRST;
    xhci_write_reg(op_regs_base, USBCMD_OFFSET, usbcmd);

    // Tunggu reset selesai: controller harus menghapus bit HCRST ketika reset selesai
    int reset_timeout = 100000; // timeout yang disesuaikan
    while ((xhci_read_reg(op_regs_base, USBCMD_OFFSET) & USBCMD_HCRST) && reset_timeout--)
        ;  // bisa ditambahkan delay pendek jika diperlukan
    if (reset_timeout <= 0) {
        // Reset gagal/tidak selesai dalam waktu yang diharapkan
        return -1;
    }

    // 3. Set Run Bit untuk menjalankan controller    // WORK        
    usbcmd = xhci_read_reg(op_regs_base, USBCMD_OFFSET);
    usbcmd |= USBCMD_RUN;
    xhci_write_reg(op_regs_base, USBCMD_OFFSET, usbcmd);

    // 4. Inisialisasi Command Ring tanpa malloc (gunakan array statis)
    uint32_t cmd_ring_phys_addr = (uint32_t)&cmd_ring; // Di kernel nyata, lakukan translasi fisik
    xhci_set_crcr(op_regs_base, cmd_ring_phys_addr);

    // 5. (Setup Event Ring)
    // Biasanya, event ring perlu diprogram lewat Interrupter Register Set dan Event Ring Segment Table.
    // Untuk contoh sederhana, asumsikan evt_ring telah dialokasikan dan hardware mengisi evt_ring.

    // 6. Siapkan sebuah TRB perintah NOOP untuk demo
    xhci_trb_t noop_trb = {0};
    noop_trb.parameter_low  = 0;
    noop_trb.parameter_high = 0;
    noop_trb.status = 0;
    // Set tipe TRB ke NOOP; cycle bit diset melalui helper
    noop_trb.control = make_trb_control(TRB_TYPE_NOOP, 1);

    // Masukkan TRB NOOP ke command ring
    enqueue_cmd_trb(&noop_trb);

    // 7. Ring doorbell pada index 0 untuk memberi tahu bahwa ada perintah baru
    xhci_ring_doorbell(op_regs_base, 0);

    // 8. Tunggu penyelesaian perintah melalui event ring.
    // Untuk contoh ini, polling event ring secara sederhana. Di implementasi nyata, gunakan mekanisme interrupt.
    volatile int timeout = 100000000;
    while (timeout--) {
        // Baca TRB dari event ring pada posisi dequeue
        xhci_trb_t evt = evt_ring[evt_ring_dequeue];
        // Cek apakah event TRB telah diperbarui dengan cycle bit yang sesuai
        if ((evt.control & 1) == evt_cycle) {
            printf("SUKSESSS \n");
            // Misalnya, event NOOP selesai sudah diterima.
            evt_ring_dequeue++;
            if (evt_ring_dequeue >= TRB_RING_SIZE) {
                evt_ring_dequeue = 0;
                evt_cycle ^= 1;
            }
            return 0; // Inisialisasi dan perintah berhasil diproses
        }
    }

    // Jika timeout tercapai tanpa menerima event, kembalikan error.
    return -1;
}
