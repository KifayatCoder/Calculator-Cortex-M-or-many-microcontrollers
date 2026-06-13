/* ============================================================
 * main.c  —  Calculator 1.0 (universal MPS2 QEMU build)
 *
 * The banner and prompt lines pull BOARD_NAME / BOARD_CPU /
 * BOARD_QEMU_M from board.h so the same source compiles for
 * every supported target without modification.
 *
 * QEMU launch (example):
 *   qemu-system-arm -M mps2-an386 -kernel calc_an386.elf -nographic
 * ============================================================ */

#define _GNU_SOURCE

#include "board.h"
#include "uart.h"
#include "calc.h"

#include <string.h>
#include <ctype.h>
#include <math.h>

#ifndef M_PI
#  define M_PI  3.14159265358979323846
#endif
#ifndef M_E
#  define M_E   2.71828182845904523536
#endif

/* ---- ANSI helpers ----------------------------------------- */
#define A_RST   "\033[0m"
#define A_BOLD  "\033[1m"
#define A_DIM   "\033[2m"
#define A_RED   "\033[31m"
#define A_GRN   "\033[32m"
#define A_YLW   "\033[33m"
#define A_CYN   "\033[36m"
#define A_WHT   "\033[97m"

/* ---- Trim leading + trailing whitespace in place ---------- */
static char *_trim(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    if (!*s) return s;
    char *e = s + strlen(s) - 1;
    while (e > s && isspace((unsigned char)*e)) *e-- = '\0';
    return s;
}

/* ---- Banner ---------------------------------------------- */
static void print_banner(void)
{
    uart_puts(A_RST "\n");
    uart_puts(A_CYN A_BOLD);
    uart_puts("  ╔══════════════════════════════════════════════════╗\n");
    uart_puts("  ║           CALCULATOR 1.0  —  ARM MPS2+           ║\n");
    uart_puts("  ╚══════════════════════════════════════════════════╝\n");
    uart_puts(A_RST A_DIM);
    uart_puts("  FPGA image : " A_WHT BOARD_NAME A_DIM
              "  (" BOARD_CPU ")\n");
    uart_puts("  QEMU target: " A_WHT BOARD_QEMU_M  A_DIM "\n");
    uart_printf("  UART0      : %lu Hz clock  →  %lu baud\n",
                (unsigned long)SYSCLK_HZ, (unsigned long)BAUD_RATE);
#if HAS_FPU
    uart_puts("  FPU        : " A_WHT "enabled (hardware)" A_DIM "\n");
#else
    uart_puts("  FPU        : " A_WHT "none (software float)" A_DIM "\n");
#endif
    uart_puts(A_RST "\n");
    uart_puts("  Type an expression and press " A_BOLD "Enter"
              A_RST " to evaluate.\n");
    uart_puts("  Type " A_YLW A_BOLD "help" A_RST " for the function list.\n\n");
}

/* ---- Help ------------------------------------------------- */
static void print_help(void)
{
    uart_puts(A_CYN A_BOLD "\n  COMMANDS\n" A_RST);
    uart_puts("  ─────────────────────────────────────────────\n");
    uart_puts("  help           Show this help\n");
    uart_puts("  history        Show last 8 calculations\n");
    uart_puts("  clear          Clear history\n");
    uart_puts("  banner         Redisplay welcome screen\n");
    uart_puts("  quit / exit    Halt processor\n\n");

    uart_puts(A_CYN A_BOLD "  OPERATORS  (precedence: high→low)\n" A_RST);
    uart_puts("  ─────────────────────────────────────────────\n");
    uart_puts("  ^    Exponentiation (right-associative)\n");
    uart_puts("  * /  Multiply, Divide\n");
    uart_puts("  + -  Add, Subtract\n");
    uart_puts("  ( )  Grouping\n\n");

    uart_puts(A_CYN A_BOLD "  CONSTANTS\n" A_RST);
    uart_puts("  ─────────────────────────────────────────────\n");
    uart_printf("  pi   =  %.15f\n", M_PI);
    uart_printf("  e    =  %.15f\n", M_E);
    uart_puts("  ans  =  last successful result\n\n");

    uart_puts(A_CYN A_BOLD "  FUNCTIONS\n" A_RST);
    uart_puts("  ─────────────────────────────────────────────\n");
    uart_puts("  sqrt(x)   Square root\n");
    uart_puts("  abs(x)    Absolute value\n");
    uart_puts("  floor(x)  Round toward -∞\n");
    uart_puts("  ceil(x)   Round toward +∞\n");
    uart_puts("  round(x)  Round to nearest\n");
    uart_puts("  sin(x)    Sine (radians)\n");
    uart_puts("  cos(x)    Cosine (radians)\n");
    uart_puts("  tan(x)    Tangent (radians)\n");
    uart_puts("  asin(x)   Arcsine\n");
    uart_puts("  acos(x)   Arccosine\n");
    uart_puts("  atan(x)   Arctangent\n");
    uart_puts("  exp(x)    e^x\n");
    uart_puts("  log(x)    Natural log\n");
    uart_puts("  log2(x)   Base-2 log\n");
    uart_puts("  log10(x)  Base-10 log\n\n");

    uart_puts(A_CYN A_BOLD "  EXAMPLES\n" A_RST);
    uart_puts("  ─────────────────────────────────────────────\n");
    uart_puts("  2 + 3 * 4         →  14\n");
    uart_puts("  (2 + 3) * 4       →  20\n");
    uart_puts("  2^10              →  1024\n");
    uart_puts("  sqrt(2)           →  1.4142135623...\n");
    uart_puts("  sin(pi / 6)       →  0.5\n");
    uart_puts("  log(e ^ 3)        →  3\n");
    uart_puts("  ans * 2           →  (last result × 2)\n\n");
}

/* ---- Print a double result with smart integer detection --- */
static void print_result(double val)
{
    uart_puts(A_GRN A_BOLD "  = ");
    if (isnan(val)) { uart_puts("NaN");           }
    else if (isinf(val)) { uart_puts(val>0 ? "+Infinity" : "-Infinity"); }
    else {
        double ip;
        if (modf(val, &ip) == 0.0 && fabs(val) < 1e15)
            uart_printf("%.0f", val);
        else
            uart_printf("%.10g", val);
    }
    uart_puts(A_RST "\n");
}

/* ---- Print error with caret ------------------------------ */
static void print_error(const char *expr, CalcErr err, int col)
{
    uart_puts(A_RED "  Error: ");
    uart_puts(CALC_ERR_STR[err]);
    uart_puts(A_RST "\n");
    if (col >= 0) {
        uart_puts("  ");
        for (int i = 0; i < col + 2; i++) uart_putc(' ');
        uart_puts(A_YLW "^\n");
        uart_puts("  ");
        for (int i = 0; i < col + 2; i++) uart_putc(' ');
        uart_puts(A_DIM);
        uart_puts(expr);
        uart_puts(A_RST "\n");
    }
}

/* ============================================================
 * main
 * ============================================================ */
int main(void)
{
    uart_init();
    print_banner();

    static char line[192];

    for (;;) {
        uart_puts(A_BOLD A_WHT "  calc> " A_RST);

        int len = uart_readline(line, sizeof(line));
        if (!len) continue;

        char *cmd = _trim(line);
        if (!*cmd) continue;

        /* ---- Commands ---- */
        if (!strcmp(cmd,"help") || !strcmp(cmd,"?"))                { print_help();                continue; }
        if (!strcmp(cmd,"history") || !strcmp(cmd,"hist"))          { calc_hist_print();            continue; }
        if (!strcmp(cmd,"clear") || !strcmp(cmd,"cls"))             { calc_hist_clear();
                                                                      uart_puts("  History cleared.\n\n"); continue; }
        if (!strcmp(cmd,"banner"))                                  { print_banner();               continue; }
        if (!strcmp(cmd,"quit")||!strcmp(cmd,"exit")||!strcmp(cmd,"q")) {
            uart_puts(A_CYN "\n  Goodbye.\n" A_RST);
            __asm volatile("cpsid i\n wfi":::"memory");
            for (;;) {}
        }

        /* ---- Evaluate ---- */
        double  result  = 0.0;
        int     err_col = -1;
        CalcErr err     = calc_eval(cmd, &result, &err_col);

        if (err == CALC_OK) {
            print_result(result);
            calc_hist_add(cmd, result);
        } else {
            print_error(cmd, err, err_col);
        }
        uart_putc('\n');
    }
}
