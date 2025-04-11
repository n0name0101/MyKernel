ENTRY(kernelMain)

SECTIONS {
    . = 0x10000;

    .text : {
        *(.text*)
    }

    .data : {
        start_ctors = . ;
        *(.data)
        *(.rodata)
        end_ctors = .;
    }

    .endofprogram : {
        KEEP(*(.endofprogram))  /* Jaga agar tidak dioptimasi */
    }

    .bss : {
        *(.bss)
    }

    /DISCARD/ : {
        *(.fini_array*)
        *(.comment)
    }
}
