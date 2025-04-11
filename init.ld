ENTRY(init_16bit)

SECTIONS {
    /* Tempatkan kernel mulai dari alamat 0x100000 */
    . = 0x0500;

    .text : {
        *(.text*)
    }

    dataSectionStartPtr = .;
    .data : {
        start_ctors = . ;
        *(.data)
        *(.rodata)
        end_ctors = .;
    }
    dataSectionEndPtr = .;


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
