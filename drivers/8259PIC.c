#include <drivers/8259PIC.h>
#include <drivers/TimerPIT.h>
#include <ioport.h>
#include <stdio.h>
#include <types.h>

//8259PICHandler.s      CPU Exception
extern void HandleException00();
extern void HandleException01();
extern void HandleException02();
extern void HandleException03();
extern void HandleException04();
extern void HandleException05();
extern void HandleException06();
extern void HandleException07();
extern void HandleException08();
extern void HandleException09();
extern void HandleException0A();
extern void HandleException0B();
extern void HandleException0C();
extern void HandleException0D();
extern void HandleException0E();
extern void HandleException0F();

extern void HandleException10();
extern void HandleException11();
extern void HandleException12();
extern void HandleException13();
extern void HandleException14();
extern void HandleException15();
extern void HandleException16();
extern void HandleException17();
extern void HandleException18();
extern void HandleException19();
extern void HandleException1A();
extern void HandleException1B();
extern void HandleException1C();
extern void HandleException1D();
extern void HandleException1E();
extern void HandleException1F();


//8259PICHandler.s      PIC 8259 Handler
extern void HandleInterruptRequest30();      // Unknown Interrupt
extern void HandleInterruptRequest20();
extern void HandleInterruptRequest21();      // For Keyboard
extern void HandleInterruptRequest22();
extern void HandleInterruptRequest23();
extern void HandleInterruptRequest24();
extern void HandleInterruptRequest25();
extern void HandleInterruptRequest26();
extern void HandleInterruptRequest27();
extern void HandleInterruptRequest28();
extern void HandleInterruptRequest29();
extern void HandleInterruptRequest2A();
extern void HandleInterruptRequest2B();
extern void HandleInterruptRequest2C();
extern void HandleInterruptRequest2D();
extern void HandleInterruptRequest2E();
extern void HandleInterruptRequest2F();

// // Handler
// extern void keyboardHandler(void);
// extern void mouseHandler(void);

// Global Data
struct GateDescriptor interruptDescriptorTable[256];
struct InterruptDestciptorPointer idt;
static void (*interruptHandler[256])(void);

uint32_t handleInt8259(uint8_t , uint32_t);
void interruptDeactive(void);
static void setInterruptDescriptorTableEntry(struct GateDescriptor *, uint16_t ,void(*)(void), uint8_t ,uint8_t );
static void interruptHandlerInit(uint8_t , void *);

uint32_t handleInt8259( uint8_t _intNum , uint32_t esp) {
   if(interruptHandler[_intNum] != NULL)
      (*interruptHandler[_intNum])();
   else {
      //printf("\n Unreconized Interrupt : %.2x \n", _intNum);
   }

   if(0x00 <= _intNum && _intNum < 0x20) {
      printf("\n Unreconized Exception : %.2x \n", _intNum);
   }

   if(_intNum == 0x30) {
      printf("\n Unreconized Interrupt : %.2x \n", _intNum);
   }

   if(0x20 <= _intNum && _intNum < 0x30) {
      outb(0x20 , 0x20);
      if(0x28 <= _intNum)
         outb(0xA0 , 0x20);
   }
   return esp;
}

static void interruptHandlerInit(uint8_t _index, void *_address) {
    interruptHandler[_index] = _address;
    return;
}

static void setInterruptDescriptorTableEntry(struct GateDescriptor *_interrupt, uint16_t _codeSegmentOffset, void (*_handler)(void), uint8_t _desctiptorPrivilegeLevel,uint8_t _descriptorType) {
    const uint8_t IDT_DESC_PRESENT = 0x80;

    _interrupt->handlerAddressLowBits = ((uint32_t) _handler) & 0xFFFF;
    _interrupt->handlerAddressHighBits = (((uint32_t)_handler) >> 16 ) & 0xFFFF;
    _interrupt->gdt_codeSegmentSelector   = _codeSegmentOffset;
    _interrupt->access = IDT_DESC_PRESENT | _descriptorType | ((_desctiptorPrivilegeLevel & 3) << 5);
    _interrupt->reserved = 0;
}

static void kb_wait(void) {
	int i;
	for (i=0; i<0x10000; i++)
		if ((inb(0x64) & 0x02) == 0)
			break;
}

static void send_cmd(unsigned char c) {
	kb_wait();
	outb(0x64,c);
}

void keyboardHandler(void) {
   static uint8_t scanCode = 0;

   send_cmd(0xAD);		/* disable keyboard */
   kb_wait();
   if ((inb(0x64) & 0x01) != 0x01)
     goto end_kbd_intr;

   uint8_t readData = inb(0x60);

   if((readData & 0x80) == 0x80)
      scanCode = 0;
   else {
      if (readData == 28)
         printf("\n");
      else {
         printf("%d", readData);
      }
      scanCode = readData;

   }

end_kbd_intr:
   send_cmd(0xAE);         /* enable keyboard */
}

int32_t init8259PIC(uint16_t CodeSegment) {
   const uint8_t IDT_INTERRUPT_GATE = 0xE;

   for(uint16_t iCounter = 0 ; iCounter < 256 ; iCounter++) {
      interruptHandler[iCounter] = NULL;
      setInterruptDescriptorTableEntry(&interruptDescriptorTable[iCounter], CodeSegment , &HandleInterruptRequest30 , 0 , IDT_INTERRUPT_GATE);
   }

   // //Set Exception
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x00], CodeSegment , &HandleException00, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x01], CodeSegment , &HandleException01, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x02], CodeSegment , &HandleException02, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x03], CodeSegment , &HandleException03, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x04], CodeSegment , &HandleException04, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x05], CodeSegment , &HandleException05, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x06], CodeSegment , &HandleException06, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x07], CodeSegment , &HandleException07, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x08], CodeSegment , &HandleException08, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x09], CodeSegment , &HandleException09, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x0A], CodeSegment , &HandleException0A, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x0B], CodeSegment , &HandleException0B, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x0C], CodeSegment , &HandleException0C, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x0D], CodeSegment , &HandleException0D, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x0E], CodeSegment , &HandleException0E, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x0F], CodeSegment , &HandleException0F, 0 , IDT_INTERRUPT_GATE);

   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x10], CodeSegment , &HandleException10, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x11], CodeSegment , &HandleException11, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x12], CodeSegment , &HandleException12, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x13], CodeSegment , &HandleException13, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x14], CodeSegment , &HandleException14, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x15], CodeSegment , &HandleException15, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x16], CodeSegment , &HandleException16, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x17], CodeSegment , &HandleException17, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x18], CodeSegment , &HandleException18, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x19], CodeSegment , &HandleException19, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x1A], CodeSegment , &HandleException1A, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x1B], CodeSegment , &HandleException1B, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x1C], CodeSegment , &HandleException1C, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x1D], CodeSegment , &HandleException1D, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x1E], CodeSegment , &HandleException1E, 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x1F], CodeSegment , &HandleException1F, 0 , IDT_INTERRUPT_GATE);

   //Set the Interrupt Entry
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x20], CodeSegment , &HandleInterruptRequest20 , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x21], CodeSegment , &HandleInterruptRequest21 , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x22], CodeSegment , &HandleInterruptRequest22 , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x23], CodeSegment , &HandleInterruptRequest23 , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x24], CodeSegment , &HandleInterruptRequest24 , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x25], CodeSegment , &HandleInterruptRequest25 , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x26], CodeSegment , &HandleInterruptRequest26 , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x27], CodeSegment , &HandleInterruptRequest27 , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x28], CodeSegment , &HandleInterruptRequest28 , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x29], CodeSegment , &HandleInterruptRequest29 , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x2A], CodeSegment , &HandleInterruptRequest2A , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x2B], CodeSegment , &HandleInterruptRequest2B , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x2C], CodeSegment , &HandleInterruptRequest2C , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x2D], CodeSegment , &HandleInterruptRequest2D , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x2E], CodeSegment , &HandleInterruptRequest2E , 0 , IDT_INTERRUPT_GATE);
   setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x2F], CodeSegment , &HandleInterruptRequest2F , 0 , IDT_INTERRUPT_GATE);
   // setInterruptDescriptorTableEntry(&interruptDescriptorTable[0x2C], CodeSegment , &HandleInterruptRequest0C , 0 , IDT_INTERRUPT_GATE);

   //Initialize The Interrupt That Hava Handler
   interruptHandlerInit(0x20,pitHandler);                 //For Keyboard From TimerPIT.c
   interruptHandlerInit(0x21,keyboardHandler);            //For Keyboard From keyboard.c
   // interruptHandlerInit(0x2C ,mouseHandler);           //For Mouse    From mouse.c

   // Set The idt address : Base and Size
   idt.size = 256*sizeof(struct GateDescriptor)-1;
   idt.base = (uint32_t)&interruptDescriptorTable;

   // PIC INIT         Master Port = 0x20,0x21    Slaver Port = 0xA0,0xA1          PIC : Programmable Interrupt Controller
   outb(0x20 , 0x11);  io_wait(); //Master
   outb(0xA0 , 0x11);  io_wait(); //Slaver

   outb(0x21 , 0x20);  io_wait(); //Master
   outb(0xA1 , 0x28);  io_wait(); //Slaver

   outb(0x21 , 0x04);  io_wait();  //......
   outb(0xA1 , 0x02);  io_wait();  //......

   outb(0x21 , 0x01);  io_wait();
   outb(0xA1 , 0x01);  io_wait();

   outb(0x21 , 0x00);  io_wait();
   outb(0xA1 , 0x00);  io_wait();

   // Verifikasi: baca kembali register mask
   uint8_t master_mask = inb(0x21);
   uint8_t slave_mask = inb(0xA1);

   // Cek apakah kedua register mask bernilai 0, jika tidak, terdapat kemungkinan kesalahan
   if (master_mask != 0x00 || slave_mask != 0x00)
      return -1;

   asm volatile("lidt %0": : "m" (idt));

   return 0;
}

void interruptActive(void) {
    asm ("sti");        //Start The interrupt
}

void interruptDeactive(void) {
    asm ("cli");        //Close The interrupt
}

void interruptRun(void) {
    asm volatile("int $0x23");
}
