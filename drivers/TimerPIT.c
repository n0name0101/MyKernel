#include <drivers/TimerPIT.h>
#include <ioport.h>
#include <utils.h>

/* Batas maksimal event yang dapat disimpan secara statis */
#define MAX_TIMER_EVENTS 128

/* Array statis untuk menyimpan event timer */
static timer_event_t timer_events[MAX_TIMER_EVENTS] = {0};
/* Global tick counter (32-bit) – setiap tick mewakili 1 ms */
volatile uint32_t ticks = 0;

/*
 * Inisialisasi PIT dengan frekuensi yang diinginkan.
 * Untuk 1 ms per tick, frequency = 1000 Hz.
 */
void initPIT(uint32_t frequency) {
   // Initialize timer_events
   memset(timer_events, 0, sizeof(timer_events));
   uint16_t divisor = PIT_BASE_FREQUENCY / frequency;

   /* Command: Channel 0, access LSB then MSB, mode 3 (square wave), binary */
   outb(PIT_COMMAND, 0x36); io_wait();
   outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF)); io_wait();           // Kirim low byte
   outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF)); io_wait();    // Kirim high byte
}

/*
 * Fungsi untuk menambahkan event timer.
 * delay_ms : berapa milidetik dari waktu sekarang event akan dipicu.
 * callback : fungsi yang akan dipanggil saat event jatuh tempo.
 * data     : parameter untuk callback.
 *
 * Return 0 jika sukses, -1 jika array penuh.
 */
int32_t pit_add_event(uint32_t delay_ms, timer_callback_t callback, void *data) {
    int32_t i;
    for (i = 0; i < MAX_TIMER_EVENTS; i++) {
        if (!timer_events[i].used) {
            timer_events[i].trigger_time = ticks + delay_ms;
            timer_events[i].callback = callback;
            timer_events[i].data = data;
            timer_events[i].used = 1;
            return 0;  // Sukses
        }
    }
    return -1; // Tidak ada slot kosong, return error
}

/*
 * Fungsi untuk mendapatkan waktu (dalam milidetik) sejak boot.
 */
uint32_t pit_get_ms() {
    return ticks;
}

/*
 * Interrupt Service Routine (ISR) untuk PIT.
 * Fungsi ini dipanggil setiap 1 ms (atau sesuai frekuensi yang diset).
 */
void pitHandler(void ) {
    // Naikkan tick global
    ticks++;

    // Periksa event-event timer yang jatuh tempo dan eksekusi callback-nya
    for (int32_t i = 0; i < MAX_TIMER_EVENTS; i++) {
        if (timer_events[i].used && (timer_events[i].trigger_time <= ticks)) {
            timer_events[i].callback(timer_events[i].data);
            timer_events[i].used = 0;  // Tandai slot sebagai kosong
        }
    }
}
