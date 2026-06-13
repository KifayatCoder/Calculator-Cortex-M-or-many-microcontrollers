/* ============================================================
 * board.h  —  Universal board configuration header
 *
 * Supported boards (pass -DBOARD_xxx via Makefile):
 *
 *  MPS2+ / CMSDK APB UART (0x40004000)
 *    BOARD_AN385   Cortex-M3           mps2-an385
 *    BOARD_AN386   Cortex-M4 + FPv4    mps2-an386
 *    BOARD_AN500   Cortex-M7 + FPv5    mps2-an500
 *    BOARD_AN505   Cortex-M33 + FPv5   mps2-an505  (IoTKit 0x50200000)
 *    BOARD_AN511   Cortex-M3 DS        mps2-an511
 *
 *  Luminary Micro Stellaris UART (0x4000C000)
 *    BOARD_LM3S811   Cortex-M3  lm3s811evb   64 KB flash /  8 KB RAM
 *    BOARD_LM3S6965  Cortex-M3  lm3s6965evb  256 KB flash / 64 KB RAM
 *
 *  ST STM32 USART (0x40013800)
 *    BOARD_STM32VL   Cortex-M3  stm32vldiscovery  128 KB flash / 8 KB RAM
 *
 * Every board exports:
 *   BOARD_NAME       Human-readable FPGA/SoC name    (string literal)
 *   BOARD_CPU        CPU name                        (string literal)
 *   BOARD_QEMU_M     QEMU -M argument                (string literal)
 *   SYSCLK_HZ        Core clock frequency            (uint32_t)
 *   BAUD_RATE        UART baud rate                  (uint32_t)
 *   HAS_FPU          1 = hardware FPU, 0 = soft-float
 *   FPU_CPACR_BITS   Bits to OR into SCB->CPACR (0 if no FPU)
 *   UART_DRIVER      UART_CMSDK | UART_STELLARIS | UART_STM32
 *   UART0_BASE       UART peripheral base address
 *
 * Additional per-family exports:
 *   UART_CMSDK:     UART_BAUDDIV_VAL
 *   UART_STELLARIS: UART_IBRD, UART_FBRD
 *   UART_STM32:     UART_BRR_VAL, STM32_VTOR_VAL (flash base for VTOR)
 * ============================================================ */

#ifndef BOARD_H
#define BOARD_H

/* ---- UART driver identifiers ----------------------------- */
#define UART_CMSDK      1
#define UART_STELLARIS  2
#define UART_STM32      3

/* ============================================================
 * MPS2+ BOARDS
 * ============================================================ */

/* ---- AN385 : Cortex-M3 ----------------------------------- */
#if defined(BOARD_AN385)
#  define BOARD_NAME     "AN385"
#  define BOARD_CPU      "Cortex-M3"
#  define BOARD_QEMU_M   "mps2-an385"
#  define UART0_BASE     0x40004000UL
#  define SYSCLK_HZ      25000000UL
#  define HAS_FPU        0
#  define UART_DRIVER    UART_CMSDK

/* ---- AN386 : Cortex-M4 + FPv4-SP ------------------------- */
#elif defined(BOARD_AN386)
#  define BOARD_NAME     "AN386"
#  define BOARD_CPU      "Cortex-M4"
#  define BOARD_QEMU_M   "mps2-an386"
#  define UART0_BASE     0x40004000UL
#  define SYSCLK_HZ      25000000UL
#  define HAS_FPU        1
#  define FPU_CPACR_BITS (0xFUL << 20)
#  define UART_DRIVER    UART_CMSDK

/* ---- AN500 : Cortex-M7 + FPv5-D16 ----------------------- */
#elif defined(BOARD_AN500)
#  define BOARD_NAME     "AN500"
#  define BOARD_CPU      "Cortex-M7"
#  define BOARD_QEMU_M   "mps2-an500"
#  define UART0_BASE     0x40004000UL
#  define SYSCLK_HZ      25000000UL
#  define HAS_FPU        1
#  define FPU_CPACR_BITS (0xFUL << 20)
#  define UART_DRIVER    UART_CMSDK

/* ---- AN505 : Cortex-M33 + FPv5-SP (IoTKit) --------------- */
#elif defined(BOARD_AN505)
#  define BOARD_NAME     "AN505"
#  define BOARD_CPU      "Cortex-M33"
#  define BOARD_QEMU_M   "mps2-an505"
#  define UART0_BASE     0x50200000UL   /* Secure APB UART0 */
#  define SYSCLK_HZ      25000000UL
#  define HAS_FPU        1
#  define FPU_CPACR_BITS (0xFUL << 20)
#  define UART_DRIVER    UART_CMSDK

/* ---- AN511 : Cortex-M3 DesignStart ----------------------- */
#elif defined(BOARD_AN511)
#  define BOARD_NAME     "AN511"
#  define BOARD_CPU      "Cortex-M3 (DesignStart)"
#  define BOARD_QEMU_M   "mps2-an511"
#  define UART0_BASE     0x40004000UL
#  define SYSCLK_HZ      25000000UL
#  define HAS_FPU        0
#  define UART_DRIVER    UART_CMSDK

/* ============================================================
 * LUMINARY MICRO STELLARIS BOARDS
 *
 * Clock: Both LM3S boards run at 8 MHz (internal oscillator)
 *        in QEMU by default.  No PLL is configured here.
 *
 * UART0 (0x4000C000) pinout: PA0 = RX, PA1 = TX.
 *
 * Baud-rate divisor (8 MHz / 115200):
 *   BRD  = 8000000 / (16 * 115200) = 4.340
 *   IBRD = 4
 *   FBRD = round(0.340 * 64) = 22
 * ============================================================ */

/* ---- LM3S811EVB : Cortex-M3, 64 KB flash, 8 KB SRAM ----- */
#elif defined(BOARD_LM3S811)
#  define BOARD_NAME     "LM3S811EVB"
#  define BOARD_CPU      "Cortex-M3"
#  define BOARD_QEMU_M   "lm3s811evb"
#  define UART0_BASE     0x4000C000UL
#  define SYSCLK_HZ      8000000UL
#  define HAS_FPU        0
#  define UART_DRIVER    UART_STELLARIS

/* ---- LM3S6965EVB : Cortex-M3, 256 KB flash, 64 KB SRAM -- */
#elif defined(BOARD_LM3S6965)
#  define BOARD_NAME     "LM3S6965EVB"
#  define BOARD_CPU      "Cortex-M3"
#  define BOARD_QEMU_M   "lm3s6965evb"
#  define UART0_BASE     0x4000C000UL
#  define SYSCLK_HZ      8000000UL
#  define HAS_FPU        0
#  define UART_DRIVER    UART_STELLARIS

/* ============================================================
 * ST STM32VL DISCOVERY
 *
 * STM32F100RBT6B: Cortex-M3, 128 KB flash @ 0x08000000,
 *                  8 KB SRAM  @ 0x20000000.
 *
 * SYSCLK = 8 MHz (HSI oscillator, no PLL configured in startup).
 * USART1 @ 0x40013800 (APB2), pins PA9=TX PA10=RX.
 *
 * STM32 BRR calculation (8 MHz / 115200):
 *   Mantissa  = 8000000 / 115200        = 69 (0x45)
 *   Fraction  = round(0.44 * 16)        = 7
 *   BRR       = (69 << 4) | 7           = 0x0457
 *
 * Flash starts at 0x08000000; SCB->VTOR must be set to that
 * address so interrupts vector correctly after boot.
 * ============================================================ */
#elif defined(BOARD_STM32VL)
#  define BOARD_NAME     "STM32VLDISCOVERY"
#  define BOARD_CPU      "Cortex-M3 (STM32F100)"
#  define BOARD_QEMU_M   "stm32vldiscovery"
#  define UART0_BASE     0x40013800UL   /* USART1 */
#  define SYSCLK_HZ      8000000UL
#  define HAS_FPU        0
#  define UART_DRIVER    UART_STM32
#  define STM32_VTOR_VAL 0x08000000UL  /* flash origin for SCB->VTOR */

#else
#  error "No board selected. Pass -DBOARD_<name> (see board.h for list)."
#endif

/* ============================================================
 * Common defaults
 * ============================================================ */
#ifndef BAUD_RATE
#  define BAUD_RATE        115200UL
#endif
#ifndef HAS_FPU
#  define HAS_FPU          0
#endif
#ifndef FPU_CPACR_BITS
#  define FPU_CPACR_BITS   0UL
#endif
#ifndef STM32_VTOR_VAL
#  define STM32_VTOR_VAL   0x00000000UL
#endif

/* ============================================================
 * UART-family baud-rate constants (compile-time arithmetic)
 * ============================================================ */

/* CMSDK: single integer divider */
#define UART_BAUDDIV_VAL  ((SYSCLK_HZ + BAUD_RATE/2u) / BAUD_RATE)

/* Stellaris: integer + fractional 6-bit divider
 *   IBRD = SYSCLK / (16 * BAUD)
 *   FBRD = ((SYSCLK * 8 / BAUD) + 1) / 2  &  0x3F        */
#define UART_IBRD  (SYSCLK_HZ / (16u * BAUD_RATE))
#define UART_FBRD  (( (SYSCLK_HZ * 8u / BAUD_RATE) + 1u) / 2u & 0x3Fu)

/* STM32: 16-bit BRR  [15:4]=mantissa  [3:0]=fraction/16
 *   mantissa = SYSCLK / BAUD
 *   fraction = ((SYSCLK % BAUD) * 16 + BAUD/2) / BAUD     */
#define _STM32_MANT ((SYSCLK_HZ) / (BAUD_RATE))
#define _STM32_FRAC (((SYSCLK_HZ) % (BAUD_RATE)) * 16u + (BAUD_RATE)/2u) / (BAUD_RATE)
#define UART_BRR_VAL  ((_STM32_MANT << 4u) | (_STM32_FRAC & 0xFu))

#endif /* BOARD_H */
