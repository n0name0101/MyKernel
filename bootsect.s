/* asm/boot/boot.s - Boot sector (512 bytes)
   Assembled dengan: as --32 asm/boot/boot.s -o boot.o
*/
.code16
.org 0x0

.global _start

_start:
   cli                         # Nonaktifkan Interrupt

   /* Init All Segment Register*/
   xorw  %ax, %ax
   movw  %ax, %ds
   movw  %ax, %es
   movw  %ax, %fs
   movw  %ax, %gs

   call clear_screen

   movw   $boot_stage_msg1, %si # Muat offset string ke SI
   call   print_string

   movw   $boot_stage_msg2, %si # Muat offset string ke SI
   call   print_string

   # Load sektor 2 sampai 10 (9 sektor) ke alamat 0x2000
   xor %ax, %ax                    # Make sure ds is set to 0
   mov %ax, %ds
   cld

   xor    %bx, %bx
   movw   %bx, %es
   movw   $0x0500, %bx           # BX = 0x0500 (alamat tujuan)
   movb   $0x80, %dl             # DL = 0 (drive 0)
   movb   $0x00, %dh             # DH = 0 (head 0)
   movb   $0x00, %ch             # CH = 0 (cylinder 0)
   movb   $0x02, %cl             # CL = n (mulai dari sektor n)
   movb   $55, %al               # AL = n (baca n sektor)
   movb   $0x02, %ah             # AH = 0x02 (fungsi baca sektor)
   int    $0x13                  # Panggil BIOS disk service
   jc     load_error             # Jika terjadi error, lompat ke load_error

   movw   $complete_msg, %si # Muat offset string ke SI
   call   print_string

   # Read EDD Drive Infomation

   xorw   %ax, %ax
   movw   %ax, %ds         # Atur DS ke 0 sehingga DS:SI menunjuk ke alamat fisik 0x0000:0x7E00
   movw   $0x7E00, %si

   movw   $0x4A, %ax       # Set ukuran buffer EDD (minimal 26 bytes)
   movw   %ax, %ds:(%si)   # Tulis ukuran pada offset 0x00 dari buffer

   movb   $0x48, %ah       # Fungsi INT 13h AH=48h (EDD Get Drive Parameters)
   movb   $0x80, %dl       # Drive 0x80
   int    $0x13
   jc     no_edd           # Jika terjadi error, lompat ke label no_edd

   sti                     # Aktifkan Interrupt
   jmp    $0x00, $0x0500

no_edd:
   # Fill 0x1A bytes with zero at 0x7E00
   movw $0x1A, %cx
   xorw %si, %si           # SI = 0
   movw $0x7E00, %si

.fill_loop:
   movb $0x00, %es:(%si)
   incw %si
   loop .fill_loop

   # Print No EDD information
   movw   $edd_msg, %si # Muat offset string ke SI
   call   print_string

.hang:
   cli
   hlt
   jmp .hang

#-----------------------------------------------------------
# Rutin clear_screen:
# Menggunakan INT 0x10 (AH=0x06) untuk membersihkan layar dari (0,0)
# sampai (24,79)
#-----------------------------------------------------------
clear_screen:
   movb   $0x06, %ah           # Fungsi scroll up / clear screen
   movb   $0, %al              # 0 baris digeser (clear seluruh layar)
   movb   $0x07, %bh           # Atribut tampilan: light grey on black
   movw   $0x0000, %cx         # Koordinat atas-kiri: (0,0)
   movw   $0x184F, %dx         # Koordinat bawah-kanan: (24,79)
   int    $0x10

   # Reset posisi kursor ke (0,0)
   movb   $0x02, %ah           # Fungsi set cursor position
   movb   $0, %bh              # Page number 0
   movb   $0, %dh              # Row = 0
   movb   $0, %dl              # Column = 0
   int    $0x10
   ret

#-----------------------------------------------------------
# Rutin print_string:
# Mencetak string yang ditunjuk oleh DS:SI menggunakan INT 0x10 (AH=0x0E)
#-----------------------------------------------------------
print_string:
   movb   $0x0E, %ah            # Fungsi teletype output
.print_char:
   lodsb                       # Ambil byte dari DS:SI ke AL, SI++
   cmpb   $0, %al              # Cek apakah karakter adalah NULL
   je     .done
   int    $0x10                # Cetak karakter yang ada di AL
   jmp    .print_char
.done:
   ret

#-----------------------------------------------------------
# Penanganan load error:
# Cetak " Failed" dan loop tanpa henti
#-----------------------------------------------------------
load_error:
   movw   $failed_msg, %si     # Muat offset string ke SI
   call   print_string
   jmp    .

#-----------------------------------------------------------
# Data pesan string
#-----------------------------------------------------------
boot_stage_msg1:
   .asciz "\n\r---Bootloader Stage 1 ---\n\n\r"
boot_stage_msg2:
   .asciz "Bootloader Stage 1 ... "
complete_msg:
   .asciz " Completed \n\r"
failed_msg:
   .asciz " Failed \n\r"
edd_msg:                      # Bios Feature to get booted drive information
   .asciz "EDD NOT SUPPORTED \n\r"

/* -------------------------------------------------- */
/*   Partition Distribution.                          */
/* -------------------------------------------------- */
   .org 0x1BF
   .byte 0x3e, 0x1d, 0x06, 0x83, 0x6c, 0xd0, 0xf9, 0x50
   .byte 0xc3, 0x00, 0x00, 0xb0, 0xa4, 0x76, 0x00, 0x00, 0x00

/* ------------------------------------------------------------------- */
/*   Pastikan boot sector berukuran 512 byte.                          */
/*   Tempatkan boot signature (0xAA55) di offset 510.                  */
/* ------------------------------------------------------------------- */
    .org 510
    .word 0xAA55
