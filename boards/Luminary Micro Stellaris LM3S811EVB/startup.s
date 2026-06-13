/* ============================================================
 * startup.s  —  Universal Cortex-M startup for all 8 targets
 *
 * Assembled with -x assembler-with-cpp so #include / #if work.
 *
 * Handles three per-board differences:
 *   1. .cpu  directive  — M3 / M4 / M7 / M33
 *   2. FPU enable in Reset_Handler  — only when HAS_FPU == 1
 *   3. VTOR initialisation          — only for STM32 (flash at
 *      0x08000000; boot-alias lets SP/PC load work from 0x00000000
 *      but VTOR must point to 0x08000000 before any ISR fires)
 * ============================================================ */

#include "board.h"

/* ---- .cpu directive --------------------------------------- */
#if   defined(BOARD_AN386)
    .cpu cortex-m4
    .fpu fpv4-sp-d16
#elif defined(BOARD_AN500)
    .cpu cortex-m7
    .fpu fpv5-d16
#elif defined(BOARD_AN505)
    .cpu cortex-m33
    .fpu fpv5-sp-d16
#else
    /* AN385, AN511, LM3S811, LM3S6965, STM32VL — all Cortex-M3 */
    .cpu cortex-m3
#endif

    .syntax unified
    .thumb

/* ============================================================
 * Vector Table
 *
 * Placed at the start of FLASH by the linker (.vectors section).
 *   MPS2 / LM3S : FLASH origin = 0x00000000
 *   STM32VL      : FLASH origin = 0x08000000
 *                  (aliased to 0x00000000 on boot so the CPU
 *                   reads the correct initial SP + PC)
 * ============================================================ */
    .section .vectors, "a", %progbits
    .global _vectors
    .type   _vectors, %object

_vectors:
    .word   _estack                 /*  0  Initial Stack Pointer       */
    .word   Reset_Handler           /*  1  Reset                       */
    .word   NMI_Handler             /*  2  NMI                         */
    .word   HardFault_Handler       /*  3  Hard Fault                  */
    .word   MemManage_Handler       /*  4  MemManage  (M4/M7/M33)      */
    .word   BusFault_Handler        /*  5  Bus Fault  (M4/M7/M33)      */
    .word   UsageFault_Handler      /*  6  Usage Fault                 */
    .word   SecureFault_Handler     /*  7  Secure Fault (M33 only)     */
    .word   0                       /*  8  Reserved                    */
    .word   0                       /*  9  Reserved                    */
    .word   0                       /* 10  Reserved                    */
    .word   SVC_Handler             /* 11  SVCall                      */
    .word   DebugMon_Handler        /* 12  Debug Monitor               */
    .word   0                       /* 13  Reserved                    */
    .word   PendSV_Handler          /* 14  PendSV                      */
    .word   SysTick_Handler         /* 15  SysTick                     */
    /* Device IRQs (240 slots) */
    .rept   240
    .word   IRQ_Handler
    .endr

    .size _vectors, .-_vectors

/* ============================================================
 * Reset Handler
 * ============================================================ */
    .section .text.Reset_Handler, "ax", %progbits
    .thumb_func
    .global Reset_Handler
    .type   Reset_Handler, %function

Reset_Handler:

/* ---- 1. FPU enable (M4 / M7 / M33 only) ------------------ */
#if HAS_FPU
    ldr     r0, =0xE000ED88         /* SCB->CPACR                     */
    ldr     r1, [r0]
    ldr     r2, =FPU_CPACR_BITS     /* CP10 + CP11 full access         */
    orr     r1, r1, r2
    str     r1, [r0]
    dsb
    isb
#endif

/* ---- 2. Set VTOR (STM32: flash at 0x08000000) ------------ */
#if defined(BOARD_STM32VL)
    ldr     r0, =0xE000ED08         /* SCB->VTOR                      */
    ldr     r1, =STM32_VTOR_VAL     /* 0x08000000                     */
    str     r1, [r0]
    dsb
#endif

/* ---- 3. Copy .data : FLASH (LMA) → RAM (VMA) ------------- */
    ldr     r0, =_ldata
    ldr     r1, =_sdata
    ldr     r2, =_edata
    b       _copy_chk
_copy_lp:
    ldr     r3, [r0], #4
    str     r3, [r1], #4
_copy_chk:
    cmp     r1, r2
    blo     _copy_lp

/* ---- 4. Zero .bss ---------------------------------------- */
    ldr     r0, =_sbss
    ldr     r1, =_ebss
    mov     r2, #0
    b       _bss_chk
_bss_lp:
    str     r2, [r0], #4
_bss_chk:
    cmp     r0, r1
    blo     _bss_lp

/* ---- 5. Run application ---------------------------------- */
    bl      main
_halt:
    b       _halt

    .size Reset_Handler, .-Reset_Handler

/* ============================================================
 * Default / Weak Handler
 * ============================================================ */
    .section .text, "ax", %progbits
    .thumb_func
    .weak  Default_Handler
    .type  Default_Handler, %function
Default_Handler:
    bkpt   #0
    b      Default_Handler
    .size  Default_Handler, .-Default_Handler

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
