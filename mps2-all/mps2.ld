/* ============================================================
 * mps2.ld  —  Linker Script for all ARM MPS2+ QEMU boards
 *
 * Memory layout (identical for AN385 / AN386 / AN500 / AN505 / AN511):
 *   ZBT SSRAM1  0x00000000  4 MB  code / vectors / rodata
 *   ZBT SSRAM2  0x20000000  4 MB  data / bss / heap / stack
 *
 * AN505 runs in Secure state by default so the Secure view of
 * the same ZBT SSRAM regions applies.
 * ============================================================ */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 4M
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 4M
}

ENTRY(Reset_Handler)

_stack_size = 0x4000;   /* 16 KB stack */
_heap_size  = 0x8000;   /* 32 KB heap  */

SECTIONS
{
    /* ---- Vector table at absolute address 0x00000000 ---- */
    .vectors :
    {
        KEEP(*(.vectors))
    } > FLASH

    /* ---- Code + read-only data -------------------------- */
    .text :
    {
        *(.text.Reset_Handler)
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* ---- ARM exception unwind tables -------------------- */
    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM.exidx : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } > FLASH

    /* ---- Initialised data (LMA in FLASH, VMA in RAM) ---- */
    _ldata = LOADADDR(.data);
    .data :
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* ---- Zero-initialised data -------------------------- */
    .bss :
    {
        _sbss = .;
        *(.bss*) *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* ---- Heap ------------------------------------------- */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        _sheap = .;
        . += _heap_size;
        _eheap = .;
    } > RAM

    /* ---- Stack (grows downward) ------------------------- */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        _sstack = .;
        . += _stack_size;
        _estack = .;        /* initial SP value loaded by vector table */
    } > RAM

    /DISCARD/ :
    {
        *(.note.GNU-stack) *(.glue_7) *(.glue_7t) *(.v4_bx)
    }
}
