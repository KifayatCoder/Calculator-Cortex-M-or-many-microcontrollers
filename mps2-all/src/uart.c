/* ============================================================
 * uart.c  —  CMSDK / IoTKit APB UART polling driver
 *            Works for all five MPS2 QEMU board targets.
 * ============================================================ */

#define _GNU_SOURCE
#include "uart.h"
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

/* ============================================================
 * uart_init
 * ============================================================ */
void uart_init(void)
{
    UART0->CTRL    = 0;                          /* disable while configuring */
    UART0->BAUDDIV = UART_BAUDDIV_VAL;           /* from board.h              */
    UART0->CTRL    = UART_CTRL_TXEN | UART_CTRL_RXEN;
}

/* ============================================================
 * uart_putc  (CR inserted before every LF for terminal compat)
 * ============================================================ */
void uart_putc(char c)
{
    if (c == '\n') {
        while (UART0->STATE & UART_STATE_TXBF) {}
        UART0->DATA = '\r';
    }
    while (UART0->STATE & UART_STATE_TXBF) {}
    UART0->DATA = (uint32_t)(uint8_t)c;
}

/* ============================================================
 * uart_getc
 * ============================================================ */
char uart_getc(void)
{
    while (!(UART0->STATE & UART_STATE_RXBF)) {}
    return (char)(UART0->DATA & 0xFFu);
}

/* ============================================================
 * uart_puts
 * ============================================================ */
void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}

/* ============================================================
 * uart_readline
 * ============================================================ */
int uart_readline(char *buf, int maxlen)
{
    int  len = 0;
    char c;
    while (1) {
        c = uart_getc();
        if (c == '\r' || c == '\n') { uart_putc('\n'); break; }
        if ((c == '\b' || c == 0x7F) && len > 0) {
            uart_putc('\b'); uart_putc(' '); uart_putc('\b');
            len--;
        } else if (c >= 0x20 && c < 0x7F && len < maxlen - 1) {
            buf[len++] = c;
            uart_putc(c);
        }
    }
    buf[len] = '\0';
    return len;
}

/* ============================================================
 * Internal helpers for uart_vprintf
 * ============================================================ */

static void _rev(char *s, int len)
{
    int i = 0, j = len - 1;
    while (i < j) { char t = s[i]; s[i]=s[j]; s[j]=t; i++; j--; }
}

static int _u32str(uint32_t v, char *buf, int base, int upper)
{
    const char *lo = "0123456789abcdef";
    const char *hi = "0123456789ABCDEF";
    const char *d  = upper ? hi : lo;
    int len = 0;
    if (!v) { buf[len++] = '0'; } else { while (v) { buf[len++] = d[v%base]; v/=base; } }
    _rev(buf, len); buf[len] = '\0'; return len;
}

static int _i32str(int32_t v, char *buf)
{
    int len = 0, neg = v < 0;
    uint32_t u = neg ? (uint32_t)(-(v+1))+1u : (uint32_t)v;
    if (!u) { buf[len++]='0'; } else { while (u) { buf[len++]='0'+(u%10); u/=10; } }
    if (neg) buf[len++] = '-';
    _rev(buf, len); buf[len] = '\0'; return len;
}

static int _f64str(double val, char *buf, int prec)
{
    /* Handle NaN / Inf via bit pattern */
    union { double d; uint64_t u; } uv; uv.d = val;
    uint64_t em = (uint64_t)0x7FFull << 52;
    if ((uv.u & em) == em) {
        if (uv.u & ((1ULL<<52)-1)) { strcpy(buf,"NaN"); return 3; }
        if (uv.u >> 63)            { strcpy(buf,"-Inf"); return 4; }
        strcpy(buf,"Inf"); return 3;
    }
    int neg = val < 0.0; if (neg) val = -val;
    if (prec < 0) prec = 6; if (prec > 10) prec = 10;
    static const double p10[11]={1e0,1e1,1e2,1e3,1e4,1e5,1e6,1e7,1e8,1e9,1e10};
    val += 0.5 / p10[prec];
    uint64_t ip = (uint64_t)val; double fr = val - (double)ip;
    char ib[24]; int il = 0;
    if (!ip) { ib[il++]='0'; } else { uint64_t t=ip; while(t){ib[il++]='0'+(t%10);t/=10;} _rev(ib,il); }
    char fb[12]; int fl = 0;
    for (int i=0;i<prec;i++){fr*=10.0;int dg=(int)fr;fb[fl++]='0'+dg;fr-=dg;}
    int pos=0;
    if (neg) buf[pos++]='-';
    for (int i=0;i<il;i++) buf[pos++]=ib[i];
    if (prec>0){ buf[pos++]='.'; for(int i=0;i<fl;i++) buf[pos++]=fb[i]; }
    buf[pos]='\0'; return pos;
}

/* ============================================================
 * uart_vprintf  — core formatter (va_list version)
 * ============================================================ */
void uart_vprintf(const char *fmt, va_list ap)
{
    char buf[48];
    while (*fmt) {
        if (*fmt != '%') { uart_putc(*fmt++); continue; }
        fmt++;

        int left = 0; if (*fmt=='-'){ left=1; fmt++; }
        int width = 0; while (*fmt>='0'&&*fmt<='9') width=width*10+(*fmt++-'0');
        int prec  = 6; if (*fmt=='.'){ fmt++; prec=0; while(*fmt>='0'&&*fmt<='9') prec=prec*10+(*fmt++-'0'); }
        while (*fmt=='l'||*fmt=='h') fmt++;   /* ignore length mods */

        char spec = *fmt++;
        const char *sptr = buf; int slen = 0;

        switch (spec) {
        case 'c': buf[0]=(char)va_arg(ap,int); buf[1]='\0'; slen=1; break;
        case 's': sptr=va_arg(ap,const char*); slen=(int)strlen(sptr); break;
        case 'd': slen=_i32str((int32_t)va_arg(ap,int),buf); break;
        case 'u': slen=_u32str((uint32_t)va_arg(ap,unsigned),buf,10,0); break;
        case 'x': slen=_u32str((uint32_t)va_arg(ap,unsigned),buf,16,0); break;
        case 'X': slen=_u32str((uint32_t)va_arg(ap,unsigned),buf,16,1); break;
        case 'p': buf[0]='0';buf[1]='x';
                  slen=_u32str((uint32_t)(uintptr_t)va_arg(ap,void*),buf+2,16,0)+2;
                  break;
        case 'f': slen=_f64str(va_arg(ap,double),buf,prec); break;
        case '%': uart_putc('%'); continue;
        default:  uart_putc('?'); continue;
        }

        if (!left) for (int i=slen;i<width;i++) uart_putc(' ');
        for (int i=0;i<slen;i++) uart_putc(sptr[i]);
        if  (left) for (int i=slen;i<width;i++) uart_putc(' ');
    }
}

/* ============================================================
 * uart_printf  — variadic wrapper around uart_vprintf
 * ============================================================ */
void uart_printf(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt); uart_vprintf(fmt, ap); va_end(ap);
}

/* ============================================================
 * Newlib syscall stubs (route I/O to UART, provide heap)
 * ============================================================ */
int _write(int fd, const char *buf, int len)
{
    (void)fd; for (int i=0;i<len;i++) uart_putc(buf[i]); return len;
}

int _read(int fd, char *buf, int len)
{
    (void)fd; for (int i=0;i<len;i++) buf[i]=uart_getc(); return len;
}

int _close(int fd)  { (void)fd; return -1; }
int _fstat(int fd, struct stat *st) { (void)fd; st->st_mode=S_IFCHR; return 0; }
int _isatty(int fd) { (void)fd; return 1; }
int _lseek(int fd, int ptr, int dir) { (void)fd;(void)ptr;(void)dir; return 0; }

/* _sbrk: heap grows from _sheap toward _eheap */
extern uint8_t _sheap, _eheap;
void *_sbrk(int incr)
{
    static uint8_t *end = &_sheap;
    uint8_t *prev = end;
    if (end + incr > &_eheap) { errno = ENOMEM; return (void *)-1; }
    end += incr; return prev;
}

/* _exit / _kill / _getpid — required by newlib abort() */
void __attribute__((noreturn)) _exit(int status)
{
    uart_printf("\n[HALT] _exit(%d)\n", status);
    __asm volatile("cpsid i\n bkpt 0\n _es: wfi\n b _es\n":::"memory");
    __builtin_unreachable();
}
int _kill(int pid, int sig)  { (void)pid; (void)sig; return -1; }
int _getpid(void)            { return 1; }
