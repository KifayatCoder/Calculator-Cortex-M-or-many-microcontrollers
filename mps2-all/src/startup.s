/* ============================================================
 * startup.s  —  Universal Cortex-M startup for all MPS2 boards
 *
 * Supports (selected by preprocessor at assemble time):
 *   AN385  Cortex-M3        (no FPU)
 *   AN386  Cortex-M4        (FPv4-SP)
 *   AN500  Cortex-M7        (FPv5-D16)
 *   AN505  Cortex-M33       (FPv5-SP, TrustZone — Secure boot)
 *   AN511  Cortex-M3 DesignStart (no FPU)
 *
 * The board.h include supplies HAS_FPU and FPU_CPACR_BITS so the
 * compiler/assembler preprocessor can strip the FPU-enable block
 * for Cortex-M3 targets at assemble time (no runtime cost).
 *
 * Assembled with:  arm-none-eabi-gcc -x assembler-with-cpp
 *   so C preprocessor directives (#include, #if …) work here.
 * ============================================================ */

#include "board.h"

/* Choose .cpu directive from board selection */
#if   defined(BOARD_AN385)
    .cpu cortex-m3
#elif defined(BOARD_AN386)
    .cpu cortex-m4
    .fpu fpv4-sp-d16
#elif defined(BOARD_AN500)
    .cpu cortex-m7
    .fpu fpv5-d16
#elif defined(BOARD_AN505)
    .cpu cortex-m33
    .fpu fpv5-sp-d16
#elif defined(BOARD_AN511)
    .cpu cortex-m3
#endif

    .syntax unified
    .thumb

/* ============================================================
 * Vector Table  (placed at 0x00000000 by linker section .vectors)
 * ============================================================ */
    .section .vectors, "a", %progbits
    .type   _vectors, %object

_vectors:
    /* --- Cortex-M core exceptions (indices 0–15) --- */
    .word   _estack                 /*  0  Initial Stack Pointer       */
    .word   Reset_Handler           /*  1  Reset                       */
    .word   NMI_Handler             /*  2  Non-Maskable Interrupt      */
    .word   HardFault_Handler       /*  3  Hard Fault                  */
    .word   MemManage_Handler       /*  4  Memory Management (M4/M7)   */
    .word   BusFault_Handler        /*  5  Bus Fault         (M4/M7)   */
    .word   UsageFault_Handler      /*  6  Usage Fault       (M4/M7)   */
    .word   SecureFault_Handler     /*  7  Secure Fault      (M33)     */
    .word   0                       /*  8  Reserved                    */
    .word   0                       /*  9  Reserved                    */
    .word   0                       /* 10  Reserved                    */
    .word   SVC_Handler             /* 11  Supervisor Call             */
    .word   DebugMon_Handler        /* 12  Debug Monitor               */
    .word   0                       /* 13  Reserved                    */
    .word   PendSV_Handler          /* 14  Pendable Service Request    */
    .word   SysTick_Handler         /* 15  System Tick                 */

    /* --- Device IRQs (IRQ0 … IRQ239) — all defaulted --- */
    .rept   240
    .word   IRQ_Handler
    .endr

    .size   _vectors, .-_vectors

/* ============================================================
 * Reset Handler
 * ============================================================ */
    .section .text.Reset_Handler, "ax", %progbits
    .thumb_func
    .global Reset_Handler
    .type   Reset_Handler, %function

Reset_Handler:

/* ---- Optional FPU enable (M4 / M7 / M33 only) ------------- *
 * HAS_FPU comes from board.h via the -x assembler-with-cpp flag.
 * For M3 boards (AN385, AN511) this block is completely absent
 * from the assembled output — no runtime overhead, no CPACR write.
 * -------------------------------------------------------------- */
#if HAS_FPU
    ldr     r0, =0xE000ED88         /* SCB->CPACR address             */
    ldr     r1, [r0]
    ldr     r2, =FPU_CPACR_BITS     /* CP10+CP11 full access          */
    orr     r1, r1, r2
    str     r1, [r0]
    dsb                             /* data sync barrier              */
    isb                             /* instruction sync barrier       */
#endif

/* ---- Copy .data section: FLASH (LMA) → RAM (VMA) ---------- */
    ldr     r0, =_ldata             /* source: load-address in FLASH  */
    ldr     r1, =_sdata             /* dest:   start of .data in RAM  */
    ldr     r2, =_edata             /* end:    end   of .data in RAM  */
    b       _copy_check
_copy_loop:
    ldr     r3, [r0], #4
    str     r3, [r1], #4
_copy_check:
    cmp     r1, r2
    blo     _copy_loop

/* ---- Zero .bss section ------------------------------------ */
    ldr     r0, =_sbss
    ldr     r1, =_ebss
    mov     r2, #0
    b       _bss_check
_bss_loop:
    str     r2, [r0], #4
_bss_check:
    cmp     r0, r1
    blo     _bss_loop

/* ---- Branch to application -------------------------------- */
    bl      main

_halt:
    b       _halt                   /* Spin if main() ever returns    */

    .size   Reset_Handler, .-Reset_Handler

/* ============================================================
 * Weak default handler — all unimplemented vectors land here.
 * BKPT halts a connected debugger; otherwise spins low-power.
 * ============================================================ */
    .section .text, "ax", %progbits

    .thumb_func
    .weak   Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    bkpt    #0x00
    b       Default_Handler
    .size   Default_Handler, .-Default_Handler

/* Alias every named exception to Default_Handler (weak) */
    .weak NMI_Handler
    .thumb_set NMI_Handler,         Default_Handler

    .weak HardFault_Handler
    .thumb_set HardFault_Handler,   Default_Handler

    .weak MemManage_Handler
    .thumb_set MemManage_Handler,   Default_Handler

    .weak BusFault_Handler
    .thumb_set BusFault_Handler,    Default_Handler

    .weak UsageFault_Handler
    .thumb_set UsageFault_Handler,  Default_Handler

    .weak SecureFault_Handler
    .thumb_set SecureFault_Handler, Default_Handler

    .weak SVC_Handler
    .thumb_set SVC_Handler,         Default_Handler

    .weak DebugMon_Handler
    .thumb_set DebugMon_Handler,    Default_Handler

    .weak PendSV_Handler
    .thumb_set PendSV_Handler,      Default_Handler

    .weak SysTick_Handler
    .thumb_set SysTick_Handler,     Default_Handler

    .weak IRQ_Handler
    .thumb_set IRQ_Handler,         Default_Handler

    .end
