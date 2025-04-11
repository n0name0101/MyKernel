# Makefile untuk membangun bootsector.bin dan init.bin,
# menggabungkan dengan dd untuk menghasilkan os-image.bin,
# dan menyediakan target run.

IMAGE = os-image.bin

# Tools
AS       = as
LD       = ld
OBJCOPY  = objcopy
CC       = gcc
CFLAGS   = -m32 -I include -ffreestanding -nostdlib -c -fno-builtin -fno-exceptions -fno-leading-underscore -msse2 -ffunction-sections
LDFLAGS  = -m elf_i386

# Init (Kernel Loader)
OBJECTSINITBIN   = 	obj/initasm.o						   \
							obj/utils.o								\
							obj/init.o								\
							obj/stdio.o								\
							obj/string.o							\
							obj/drivers/xhcitest.o				\
							obj/drivers/usbxhci.o				\
							obj/drivers/pci.o						\
							obj/drivers/VGA.o						\
							obj/drivers/ahci.o					\
							obj/drivers/ata.o						\
							obj/drivers/TimerPIT.o

# Kernel
OBJECTSKERNELBIN  = 	obj/kernel.o							\
							obj/utils.o								\
							obj/stdio.o								\
							obj/string.o							\
							obj/drivers/xhcitest.o				\
							obj/drivers/usbxhci.o				\
							obj/drivers/cpuid.o					\
							obj/drivers/pci.o						\
							obj/drivers/VGA.o						\
							obj/drivers/ahci.o					\
							obj/drivers/ata.o						\
							obj/drivers/8259PIC.o				\
							obj/drivers/8259PICHandler.o		\
							obj/drivers/TimerPIT.o				\
							obj/filesystem/vfilesystem.o		\
							obj/filesystem/devtmpsys.o

.PHONY: all clean run

all: $(IMAGE)

obj:
	@echo "[+] Membuat folder obj"
	@rm -rf obj
	@mkdir -p obj/drivers
	@mkdir -p obj/filesystem

bootsect.bin: bootsect.s
	@echo "[+] Merakit bootsect.bin"
	@$(AS) -o bootsect.o bootsect.s
	@$(LD) -Ttext 0x7C00 -o bootsect.elf bootsect.o --oformat binary
	@mv bootsect.elf bootsect.bin

obj/%.o: %.c | obj
	@echo "[~] Compile $<"
	@$(CC) $(CFLAGS) -o $@ $<

obj/drivers/%.o: %.c | obj
	@echo "[~] Compile $<"
	@$(CC) $(CFLAGS) -o $@ $<

obj/%.o: %.s | obj
	@echo "[~] Assemble $<"
	@$(AS) --32 -o $@ $<

obj/drivers/%.o: %.s | obj
	@echo "[~] Assemble $<"
	@$(AS) --32 -o $@ $<

init.bin: $(OBJECTSINITBIN)
	@echo "[+] Linking init.bin"
	@$(LD) $(LDFLAGS) --gc-section -T init.ld -o init.elf $(OBJECTSINITBIN)						#--gc-sections untuk menghapus function yang tidak dipakai
	@$(OBJCOPY) -O binary init.elf init.bin

kernel.bin: $(OBJECTSKERNELBIN)
	@echo "[+] Linking kernel.bin"
	@$(LD) $(LDFLAGS) -T kernel.ld -o kernel.elf $(OBJECTSKERNELBIN)
	@$(OBJCOPY) -O binary kernel.elf kernel.bin

$(IMAGE): bootsect.bin init.bin kernel.bin
	@echo "[+] Membuat os-image.bin"
	@truncate -s 512000 $(IMAGE)
	@dd if=bootsect.bin of=$(IMAGE) bs=512 count=1 conv=notrunc status=none
	@dd if=init.bin of=$(IMAGE)     bs=512 seek=1 conv=notrunc status=none
	@dd if=kernel.bin of=$(IMAGE)   bs=28672 seek=1 conv=notrunc status=none

clean:
	@echo "[-] Membersihkan file build"
	@rm -rf bootsect.o
	@rm -rf obj
	@rm -f *.bin *.elf $(IMAGE)

run: $(IMAGE)
	@echo "[>] Menjalankan QEMU dengan ATA"
	@qemu-system-x86_64 -drive if=none,id=stick,format=raw,file=usb.img -device nec-usb-xhci,id=xhci -device usb-storage,bus=xhci.0,drive=stick $(IMAGE)
	#@qemu-system-i386 -usb -drive if=none,id=usbstick,file=usb.img,format=raw $(IMAGE)
