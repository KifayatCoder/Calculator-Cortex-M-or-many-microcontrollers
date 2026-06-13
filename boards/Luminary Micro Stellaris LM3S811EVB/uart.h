/* ============================================================
 * uart.h  —  Unified UART API (CMSDK / Stellaris / STM32)
 *
 * The implementation (uart.c) selects the correct peripheral
 * driver at compile time based on UART_DRIVER from board.h.
 * The API surface is identical for all boards.
 * ============================================================ */

#ifndef UART_H
#define UART_H

#include "board.h"
#include <stdarg.h>
#include <stdint.h>

/* ============================================================
 * Public API
 * ============================================================ */

/** Initialise UART0 / USART1 for 115200-8N1 polling. */
void uart_init(void);

/** Transmit one character.  Inserts CR before LF. */
void uart_putc(char c);

/** Receive one character (blocking). */
char uart_getc(void);

/** Transmit a NUL-terminated string. */
void uart_puts(const char *s);

/** Lightweight printf — %c %s %d %u %x %X %p %f (+width, +.prec). */
void uart_printf(const char *fmt, ...);

/** va_list variant (used by code that builds on top of uart_printf). */
void uart_vprintf(const char *fmt, va_list ap);

/** Read a line from UART with echo and backspace support.
 *  Returns number of characters stored (not counting NUL). */
int  uart_readline(char *buf, int maxlen);

/* ============================================================
 * Register structures (referenced internally; exposed so board-
 * specific code can poke peripherals if needed).
 * ============================================================ */

/* ---- CMSDK APB UART (MPS2 boards) ------------------------ */
#if UART_DRIVER == UART_CMSDK
typedef struct {
    volatile uint32_t DATA;
    volatile uint32_t STATE;
    volatile uint32_t CTRL;
    volatile uint32_t INTSTATUS;
    volatile uint32_t BAUDDIV;
} CMSDK_UART_t;
#define UART0  ((CMSDK_UART_t *)UART0_BASE)
#define UART_STATE_TXBF  (1u << 0)
#define UART_STATE_RXBF  (1u << 1)
#define UART_CTRL_TXEN   (1u << 0)
#define UART_CTRL_RXEN   (1u << 1)
#endif /* UART_CMSDK */

/* ---- Stellaris UART (LM3S boards) ------------------------ */
#if UART_DRIVER == UART_STELLARIS
typedef struct {
    volatile uint32_t DR;         /* 0x000  Data                      */
    volatile uint32_t RSR;        /* 0x004  Receive Status / ECR      */
    uint32_t          _res0[4];   /* 0x008–0x014 reserved             */
    volatile uint32_t FR;         /* 0x018  Flag                      */
    uint32_t          _res1[1];   /* 0x01C reserved                   */
    volatile uint32_t ILPR;       /* 0x020  IrDA LP Counter           */
    volatile uint32_t IBRD;       /* 0x024  Integer Baud-Rate Div     */
    volatile uint32_t FBRD;       /* 0x028  Fractional Baud-Rate Div  */
    volatile uint32_t LCRH;       /* 0x02C  Line Control              */
    volatile uint32_t CTL;        /* 0x030  Control                   */
} STELLARIS_UART_t;

#define UART0  ((STELLARIS_UART_t *)UART0_BASE)

/* FR (Flag Register) */
#define UART_FR_TXFF    (1u << 5)   /* TX FIFO full    */
#define UART_FR_RXFE    (1u << 4)   /* RX FIFO empty   */
#define UART_FR_BUSY    (1u << 3)   /* UART busy       */
/* CTL (Control Register) */
#define UART_CTL_UARTEN (1u << 0)   /* UART enable     */
#define UART_CTL_TXE    (1u << 8)   /* TX enable       */
#define UART_CTL_RXE    (1u << 9)   /* RX enable       */
/* LCRH (Line Control) */
#define UART_LCRH_WLEN8 (0x3u << 5) /* 8-bit word length */
#define UART_LCRH_FEN   (1u << 4)   /* FIFO enable       */

/* Stellaris SYSCTL (clock gating) */
#define SYSCTL_RCGC1    (*((volatile uint32_t *)0x400FE104UL))
#define SYSCTL_RCGC2    (*((volatile uint32_t *)0x400FE108UL))
#define SYSCTL_RCGC1_UART0EN  (1u << 0)
#define SYSCTL_RCGC2_GPIOA    (1u << 0)

/* GPIOA alternate-function select (PA0=RX, PA1=TX) */
#define GPIOA_AFSEL     (*((volatile uint32_t *)0x40004420UL))
#endif /* UART_STELLARIS */

/* ---- STM32 USART (STM32VL board) ------------------------- */
#if UART_DRIVER == UART_STM32
typedef struct {
    volatile uint32_t SR;         /* 0x00  Status                     */
    volatile uint32_t DR;         /* 0x04  Data                       */
    volatile uint32_t BRR;        /* 0x08  Baud Rate                  */
    volatile uint32_t CR1;        /* 0x0C  Control 1                  */
    volatile uint32_t CR2;        /* 0x10  Control 2                  */
    volatile uint32_t CR3;        /* 0x14  Control 3                  */
    volatile uint32_t GTPR;       /* 0x18  Guard time / prescaler     */
} STM32_USART_t;

#define UART0  ((STM32_USART_t *)UART0_BASE)  /* USART1 */

/* SR bits */
#define USART_SR_TXE    (1u << 7)   /* TX data register empty */
#define USART_SR_TC     (1u << 6)   /* Transmission complete  */
#define USART_SR_RXNE   (1u << 5)   /* RX data register not empty */
/* CR1 bits */
#define USART_CR1_UE    (1u << 13)  /* USART enable  */
#define USART_CR1_TE    (1u << 3)   /* TX enable     */
#define USART_CR1_RE    (1u << 2)   /* RX enable     */

/* RCC (Reset & Clock Control) */
#define RCC_BASE        0x40021000UL
#define RCC_APB2ENR     (*((volatile uint32_t *)(RCC_BASE + 0x18UL)))
#define RCC_APB2ENR_IOPAEN   (1u << 2)   /* GPIOA clock */
#define RCC_APB2ENR_USART1EN (1u << 14)  /* USART1 clock */

/* GPIOA (PA9=TX, PA10=RX) */
#define GPIOA_BASE      0x40010800UL
#define GPIOA_CRH       (*((volatile uint32_t *)(GPIOA_BASE + 0x04UL)))
/* CRH[7:4]  = PA9  → 0xB (AF push-pull, 50 MHz) */
/* CRH[11:8] = PA10 → 0x4 (floating input)        */
#endif /* UART_STM32 */

#endif /* UART_H */
