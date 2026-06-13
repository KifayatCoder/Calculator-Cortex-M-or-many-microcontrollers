/* ============================================================
 * uart.h  —  CMSDK APB UART driver (all MPS2 boards)
 *
 * The peripheral base address (UART0_BASE) and clock / baud-rate
 * divider (UART_BAUDDIV_VAL) are supplied by board.h so the same
 * driver compiles unmodified for every supported target:
 *
 *   AN385 / AN386 / AN500 / AN511 → UART0 @ 0x40004000 (CMSDK)
 *   AN505                         → UART0 @ 0x50200000 (IoTKit)
 * ============================================================ */

#ifndef UART_H
#define UART_H

#include "board.h"
#include <stdarg.h>
#include <stdint.h>

/* ---- CMSDK / IoTKit APB UART register layout -------------- */
typedef struct {
    volatile uint32_t DATA;      /**< 0x00  Tx / Rx data             */
    volatile uint32_t STATE;     /**< 0x04  Status                   */
    volatile uint32_t CTRL;      /**< 0x08  Control                  */
    volatile uint32_t INTSTATUS; /**< 0x0C  Interrupt status / clear */
    volatile uint32_t BAUDDIV;   /**< 0x10  Baud-rate divider        */
} UART_t;

/* ---- Peripheral instance (address from board.h) ----------- */
#define UART0   ((UART_t *)UART0_BASE)

/* ---- Register bit masks ----------------------------------- */
#define UART_STATE_TXBF   (1u << 0)  /**< TX buffer full              */
#define UART_STATE_RXBF   (1u << 1)  /**< RX buffer full (char ready) */
#define UART_STATE_TXOR   (1u << 2)  /**< TX overrun                  */
#define UART_STATE_RXOR   (1u << 3)  /**< RX overrun                  */

#define UART_CTRL_TXEN    (1u << 0)  /**< TX enable                   */
#define UART_CTRL_RXEN    (1u << 1)  /**< RX enable                   */
#define UART_CTRL_TXIRQEN (1u << 2)  /**< TX interrupt enable         */
#define UART_CTRL_RXIRQEN (1u << 3)  /**< RX interrupt enable         */

/* ============================================================
 * Public API
 * ============================================================ */

/** Initialise UART0 for polling 8-N-1 at BAUD_RATE. */
void uart_init(void);

/** Transmit one character (blocks until TX buffer free). */
void uart_putc(char c);

/** Receive one character (blocks until a byte arrives). */
char uart_getc(void);

/** Transmit a NUL-terminated string. */
void uart_puts(const char *s);

/**
 * Lightweight formatted output (no heap).
 * Conversions: %c %s %d %u %x %X %p %f  (+width, +.prec for %f).
 */
void uart_printf(const char *fmt, ...);

/** va_list variant — lets tui_printf forward its args. */
void uart_vprintf(const char *fmt, va_list ap);

/**
 * Read one line from UART into buf (max maxlen-1 chars).
 * Echoes typing; handles backspace (BS / DEL).
 * Returns number of characters stored (not counting NUL).
 */
int  uart_readline(char *buf, int maxlen);

#endif /* UART_H */
