/* ============================================================
 * board.h  —  MPS2 QEMU board configuration selector
 *
 * Pass -DBOARD_AN385 / -DBOARD_AN386 / -DBOARD_AN500 /
 *      -DBOARD_AN505 / -DBOARD_AN511  via the Makefile.
 *
 * Defines exported to all translation units:
 *   BOARD_NAME     Human-readable FPGA image name  (string)
 *   BOARD_CPU      CPU name                        (string)
 *   BOARD_QEMU_M   QEMU -M value                  (string)
 *   UART0_BASE     CMSDK / IoTKit UART0 address    (uint32)
 *   SYSCLK_HZ      Core clock frequency            (uint32)
 *   BAUD_RATE      UART baud rate                  (uint32)
 *   HAS_FPU        1 = FPU present, 0 = soft-float (integer)
 *   FPU_CPACR_BITS Bits to OR into SCB->CPACR      (uint32)
 *
 * QEMU machine strings:
 *   mps2-an385   Cortex-M3          (no FPU)
 *   mps2-an386   Cortex-M4 + FPv4-SP
 *   mps2-an500   Cortex-M7 + FPv5-D16
 *   mps2-an505   Cortex-M33 + FPv5-SP  (IoTKit / SSE-200)
 *   mps2-an511   Cortex-M3 DesignStart  (no FPU)
 * ============================================================ */

#ifndef BOARD_H
#define BOARD_H

/* ---- AN385 : Cortex-M3 ------------------------------------ */
#if defined(BOARD_AN385)
#  define BOARD_NAME      "AN385"
#  define BOARD_CPU       "Cortex-M3"
#  define BOARD_QEMU_M    "mps2-an385"
#  define UART0_BASE      0x40004000UL   /* CMSDK APB UART0 */
#  define SYSCLK_HZ       25000000UL
#  define HAS_FPU         0

/* ---- AN386 : Cortex-M4 + FPv4-SP (single-precision) ------- */
#elif defined(BOARD_AN386)
#  define BOARD_NAME      "AN386"
#  define BOARD_CPU       "Cortex-M4"
#  define BOARD_QEMU_M    "mps2-an386"
#  define UART0_BASE      0x40004000UL
#  define SYSCLK_HZ       25000000UL
#  define HAS_FPU         1
   /* CP10+CP11 full access = bits [23:20] all set */
#  define FPU_CPACR_BITS  (0xFUL << 20)

/* ---- AN500 : Cortex-M7 + FPv5-D16 (double-precision) ------ */
#elif defined(BOARD_AN500)
#  define BOARD_NAME      "AN500"
#  define BOARD_CPU       "Cortex-M7"
#  define BOARD_QEMU_M    "mps2-an500"
#  define UART0_BASE      0x40004000UL
#  define SYSCLK_HZ       25000000UL
#  define HAS_FPU         1
#  define FPU_CPACR_BITS  (0xFUL << 20)

/* ---- AN505 : Cortex-M33 + FPv5-SP (IoTKit / SSE-200) ------ */
/*
 * The AN505 FPGA image uses the ARM IoTKit / SSE-200 subsystem.
 * The processor starts in Secure state.  UART0 in the Secure APB
 * expansion bus is at 0x50200000 (alias 0x40200000 non-secure).
 *
 * NOTE: Verified against QEMU 8.x mps2-an505 machine model.
 *       Adjust SYSCLK_HZ if your QEMU version uses a different
 *       default system oscillator.
 */
#elif defined(BOARD_AN505)
#  define BOARD_NAME      "AN505"
#  define BOARD_CPU       "Cortex-M33"
#  define BOARD_QEMU_M    "mps2-an505"
#  define UART0_BASE      0x50200000UL   /* IoTKit UART0, Secure */
#  define SYSCLK_HZ       25000000UL
#  define HAS_FPU         1
#  define FPU_CPACR_BITS  (0xFUL << 20)

/* ---- AN511 : Cortex-M3 DesignStart (CMSDK, no FPU) -------- */
#elif defined(BOARD_AN511)
#  define BOARD_NAME      "AN511"
#  define BOARD_CPU       "Cortex-M3 (DesignStart)"
#  define BOARD_QEMU_M    "mps2-an511"
#  define UART0_BASE      0x40004000UL
#  define SYSCLK_HZ       25000000UL
#  define HAS_FPU         0

#else
#  error "No board selected.  Pass -DBOARD_AN385 / AN386 / AN500 / AN505 / AN511."
#endif

/* ---- Common defaults ------------------------------------- */
#ifndef BAUD_RATE
#  define BAUD_RATE       115200UL
#endif

#ifndef HAS_FPU
#  define HAS_FPU         0
#endif

#ifndef FPU_CPACR_BITS
#  define FPU_CPACR_BITS  0UL
#endif

/* Derived: baud-rate divider = SYSCLK / BAUD (rounded) */
#define UART_BAUDDIV_VAL  ((SYSCLK_HZ + BAUD_RATE/2) / BAUD_RATE)

#endif /* BOARD_H */
