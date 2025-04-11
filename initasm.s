.global init_16bit, x_resolution, y_resolution, vbe_physical_base_pointer

.extern initMain

.section .text      # TEXT SECTION

.code16             # 16 bit Instructions

   /*-----------------------------------------------------------*/
   /*                     16-bit CODE                           */
   /*-----------------------------------------------------------*/

init_16bit:
   /* Set up Stack */
   movw $init_16bit, %sp

   movw   $boot_stage_msg1, %si          # Muat offset string ke SI
   call   print_string

   /* Code for set-up VBE */
   xorw %ax, %ax
   movw %ax, %es                         # Reset ES ke 0 (segment register)

   movw $0x4F00, %ax                     # VBE Function 00h - Get Controller Info
   movw $vbe_info_block, %di             # Offset dari struktur info (harus seg:ofs)
   int  $0x10                            # BIOS Video Interrupt

   cmpw $0x4F, %ax                       # Jika AX != 0x4F, maka error
   jne  error

   movw video_mode_pointer, %ax          # Ambil word pertama (offset)
   movw %ax, offset

   movw video_mode_pointer+0x02, %ax     # Ambil word kedua (segment)
   movw %ax, t_segment

   movw %ax, %fs
   movw offset, %si

   find_mode:
      movw    %fs:(%si), %dx             # Ambil mode dari daftar FS:SI
      addw    $2, %si                    # SI += 2 (karena setiap mode 2 byte)
      movw    %si, offset                # Simpan offset ke variabel offset
      movw    %dx, mode                  # Simpan mode ke variabel mode

      cmpw    $0xFFFF, %dx               # Apakah akhir daftar?
      je      end_of_modes

      movw    $0x4F01, %ax               # Fungsi: Dapatkan info mode
      movw    mode, %cx                  # Mode dari variabel mode
      movw    $mode_info_block, %di      # Buffer tujuan info mode
      int     $0x10                      # Panggil BIOS VBE

      cmpw    $0x004F, %ax               # Apakah berhasil (AX = 0x4F)?
      jne     error


      /* PRINT MODE YANG TERSEDIA */
      movw   $widthMsg, %si           # Muat offset string ke SI
      call   print_string
      movw    x_resolution, %ax
      call    print_decimal

      movw   $heightMsg, %si          # Muat offset string ke SI
      call   print_string
      movw    y_resolution, %ax
      call    print_decimal

      movw   $newLine, %si          # Muat offset string ke SI
      call   print_string
      /* PRINT MODE YANG TERSEDIA */

      #call   wait_for_input

      # Bandingkan resolusi dengan yang diinginkan
      movw    width, %ax
      cmpw    x_resolution, %ax
      jne     next_mode

      movw    height, %ax
      cmpw    y_resolution, %ax
      jne     next_mode

      movb    bpp, %al
      cmpb    bits_per_pixel, %al
      jne     next_mode

      # Set VBE mode
      movw    $0x4F02, %ax
      movw    mode, %bx
      orw     $0x4000, %bx              # Aktifkan LFB
      xorw    %di, %di
      int     $0x10

      cmpw    $0x004F, %ax
      jne     error

      jmp     switch_to_protected_mode

   next_mode:
      movw    t_segment, %ax
      movw    %ax, %fs
      movw    offset, %si
      jmp     find_mode

   end_of_modes:
      jmp error

   call   clear_screen_16bit

   /* Code for jumping to 32 bit protected mode */
switch_to_protected_mode :
   cli                     # Nonaktifkan interrupt
   cld                     # Clear direction flag

   /* Load GDT */
   lgdt gdt_descriptor

   /* Enable Protected Mode:
    Baca CR0, set bit 0, tulis kembali ke CR0 */
   movl   %cr0, %eax
   orl    $0x1, %eax
   movl   %eax, %cr0

   /* Far jump ke protected_mode_entry */
   ljmp   $0x08, $init_32bit

#-----------------------------------------------------------
# error:
# Menampilkan karakter ERROR menggunakan BIOS interrupt 10h (fungsi teletype)
# lalu menghentikan sistem dengan menonaktifkan interrupt dan halt.
#-----------------------------------------------------------
error:
   movw   $error_msg, %si     # Muat offset string ke SI
   call   print_string
   cli                        # Nonaktifkan interrupt
   hlt                        # Hentikan CPU

#-----------------------------------------------------------
# wait_for_input:
# Menunggu masukan dari keyboard sebelum melanjutkan eksekusi.
# Menggunakan BIOS interrupt 16h (fungsi 0) untuk menerima input.
#-----------------------------------------------------------
wait_for_input:
   pusha                     # Simpan register umum
   movw    $prompt, %si     # Pointer ke pesan prompt
   call    print_string      # Cetak pesan "Press any key to continue..."

   movb    $0, %ah          # Fungsi BIOS 16h: Wait for keystroke
   int     $0x16            # Tunggu masukan dari keyboard

   popa                      # Kembalikan register
   ret

#-----------------------------------------------------------
# print_hex: Tampilkan DX sebagai 4 digit hex
# - Ambil 4 nibble dari DX, mulai dari LSB
# - Konversi ke karakter dengan xlatb menggunakan hex_to_ascii
# - Simpan ke hexString dan cetak ke layar
#-----------------------------------------------------------
print_hex:
   pusha
   mov     $4, %cx                    # Counter for 4 hex digits
.hex_loop:
   mov     %dx, %ax                   # Copy hex value from DX to AX
   and     $0x0F, %al                 # Mask lower nibble
   mov     $hex_to_ascii, %bx
   xlat                               # AL = [BX + AL], BX is implicitly used in xlat

   mov     %cx, %bx                   # BX = CX (used as index)
   mov     %al, hexString+1(%bx)      # Store character to string

   ror     $4, %dx                    # Rotate next nibble into AL
   loop    .hex_loop

   lea     hexString, %si             # SI points to hex string
   mov     $6, %cx                    # Length to print

   movw   $hexString, %si # Muat offset string ke SI
   call   print_string
   popa

   ret

#-----------------------------------------------------------
# Rutin clear_screen:
# Menggunakan INT 0x10 (AH=0x06) untuk membersihkan layar dari (0,0)
# sampai (24,79)
#-----------------------------------------------------------
clear_screen_16bit:
   pusha
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
   popa
   ret

#-----------------------------------------------------------
# Rutin print_string:
# Mencetak string yang ditunjuk oleh DS:SI menggunakan INT 0x10 (AH=0x0E)
#-----------------------------------------------------------
print_string:
   pusha
   movb   $0x0E, %ah            # Fungsi teletype output
.print_char:
   lodsb                       # Ambil byte dari DS:SI ke AL, SI++
   cmpb   $0, %al              # Cek apakah karakter adalah NULL
   je     .done
   int    $0x10                # Cetak karakter yang ada di AL
   jmp    .print_char
.done:
   popa
   ret

#-----------------------------------------------------------
# print_decimal:
# Mencetak nilai desimal (angka) yang ada di register AX ke layar.
# Prosedur ini mengonversi nilai ke digit-digit ASCII dengan pembagian berulang,
# kemudian mencetak digit-digit tersebut dalam urutan yang benar.
# Menggunakan BIOS interrupt 10h (fungsi teletype).
#-----------------------------------------------------------
print_decimal:
   pusha                   # Simpan register umum
   cmpw    $0, %ax
   jne     .convert_decimal
   # Jika nilai sama dengan 0, langsung cetak '0'
   movb    $'0', %al
   movb    $0x0E, %ah
   int     $0x10
   jmp     .done_decimal

.convert_decimal:
   xor    %cx, %cx      # Kosongkan CX (counter digit); gunakan 16-bit: xor %cx,%cx
.convert_loop:
   xor    %dx, %dx      # Kosongkan DX sebelum div (pastikan DX:AX adalah dividend)
   movw    $10, %bx      # Pembagi = 10
   divw    %bx           # AX = AX / 10, DX = sisa bagi
   pushw   %dx           # Simpan digit (sisa) di stack
   incw    %cx           # Hitung jumlah digit
   cmpw    $0, %ax
   jne     .convert_loop

.print_digits:
   popw    %dx           # Ambil digit dari stack
   addw    $'0', %dx     # Konversi digit menjadi karakter ASCII
   movb    %dl, %al      # Pindahkan digit ke AL
   movb    $0x0E, %ah    # Fungsi teletype BIOS
   int     $0x10         # Tampilkan karakter
   loop    .print_digits # Ulangi sebanyak jumlah digit

.done_decimal:
   popa                  # Kembalikan register
   ret

   /*-----------------------------------------------------------*/
   /*                     32-bit CODE                           */
   /*-----------------------------------------------------------*/

.code32             # 32 bit Instructions

init_32bit:
   /* Setup segmen data (selector 0x10) */
   movw   $0x10, %ax
   movw   %ax, %ds
   movw   %ax, %es
   movw   %ax, %fs
   movw   %ax, %gs
   movw   %ax, %ss
   mov    $init_stack,%esp

   call initMain

   cli
   hlt

_stop:
   cli
   hlt
   jmp _stop

.section .data     # DATA SECTION

prompt:           .string "Press any key to continue...\n\r"
widthMsg:         .string "Width : "      # String pesan untuk lebar
heightMsg:        .string ", Height : "   # String pesan untuk tinggi
newLine:          .string "\n\r"          # Baris baru

error_msg:        .string "\n\r---ERROR---\n\n\r"
boot_stage_msg1:  .string "\n\r---Bootloader Stage 2 ---\n\n\r"
message1:         .string "Load Kernel ...!\n\0"
message2:         .string "Kernel Loaded !\n\0"

/* VGA VESA MODE */

hexString:        .string "0x0000\n\r"
hex_to_ascii:     .ascii "0123456789ABCDEF"
width:            .word 1280          # .word = 2 byte
height:           .word 1024
bpp:              .byte 32
offset:           .word 0
t_segment:        .word 0
mode:             .word 0

vbe_info_block:
   vbe_signature:                   .ascii "VBE2"
   vbe_version:                     .word 0
   oem_string_pointer:              .long 0
   capabilities:                    .long 0
   video_mode_pointer:              .long 0
   total_memory:                    .word 0
   oem_software_rev:                .word 0
   oem_vendor_name_pointer:         .long 0
   oem_product_name_pointer:        .long 0
   oem_product_revision_pointer:    .long 0
   reserved:                        .space 222
   oem_data:                        .space 256

mode_info_block:
   # Mandatory info for all VBE revisions
   mode_attributes:                 .word 0
   window_a_attributes:             .byte 0
   window_b_attributes:             .byte 0
   window_granularity:              .word 0
   window_size:                     .word 0
   window_a_segment:                .word 0
   window_b_segment:                .word 0
   window_function_pointer:         .long 0
   bytes_per_scanline:              .word 0

   # Mandatory info for VBE 1.2 and above
   x_resolution:                   .word 0
   y_resolution:                   .word 0
   x_charsize:                     .byte 0
   y_charsize:                     .byte 0
   number_of_planes:               .byte 0
   bits_per_pixel:                 .byte 0
   number_of_banks:                .byte 0
   memory_model:                   .byte 0
   bank_size:                      .byte 0
   number_of_image_pages:          .byte 0
   reserved1:                      .byte 1

   # Direct color fields (required for direct/6 and YUV/7 memory models)
   red_mask_size:                 .byte 0
   red_field_position:            .byte 0
   green_mask_size:               .byte 0
   green_field_position:          .byte 0
   blue_mask_size:                .byte 0
   blue_field_position:           .byte 0
   reserved_mask_size:            .byte 0
   reserved_field_position:       .byte 0
   direct_color_mode_info:        .byte 0

   # Mandatory info for VBE 2.0 and above
   vbe_physical_base_pointer:     .long 0
   reserved2:                     .long 0
   reserved3:                     .word 0

   # Mandatory info for VBE 3.0 and above
   linear_bytes_per_scan_line:    .word 0
   bank_number_of_image_pages:    .byte 0
   linear_number_of_image_pages:  .byte 0
   linear_red_mask_size:          .byte 0
   linear_red_field_position:     .byte 0
   linear_green_mask_size:        .byte 0
   linear_green_field_position:   .byte 0
   linear_blue_mask_size:         .byte 0
   linear_blue_field_position:    .byte 0
   linear_reserved_mask_size:     .byte 0
   linear_reserved_field_position:.byte 0
   max_pixel_clock:               .long 0

   reserved4:                     .space 190

/* GDT Definitions */
gdt_start:
   .quad 0x0000000000000000         # Null descriptor
   .quad 0x00CF9A000000FFFF         # Code descriptor: base=0, limit=4GB, execute/read
   .quad 0x00CF92000000FFFF         # Data descriptor: base=0, limit=4GB, read/write
gdt_end:

gdt_descriptor:
   .word (gdt_end - gdt_start - 1)   # Limit (ukuran GDT - 1)
   .long gdt_start                   # Base address GDT

.section .bss     # BSS SECTION

.space 2*1024*1024     ; #The space for Stack about : 2 MiB
init_stack:
