/* ============================================================
 * uart.c  —  Unified polling UART driver
 *
 * Three hardware families, one API:
 *   UART_CMSDK      MPS2+ boards  (ARM CMSDK APB UART)
 *   UART_STELLARIS  LM3S boards   (Luminary/TI Stellaris UART)
 *   UART_STM32      STM32VL board (ST USART)
 *
 * Selected at compile time via UART_DRIVER (from board.h).
 * ============================================================ */

#define _GNU_SOURCE
#include "uart.h"
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

/* ============================================================
 * ---- SECTION 1: uart_init  (one per family) ---------------
 * ============================================================ */

/* ---- CMSDK (MPS2 boards) --------------------------------- */
#if UART_DRIVER == UART_CMSDK

void uart_init(void)
{
    UART0->CTRL    = 0;
    UART0->BAUDDIV = UART_BAUDDIV_VAL;
    UART0->CTRL    = UART_CTRL_TXEN | UART_CTRL_RXEN;
}

static inline int _tx_busy(void) { return UART0->STATE & UART_STATE_TXBF; }
static inline int _rx_ready(void){ return UART0->STATE & UART_STATE_RXBF; }
static inline void _tx_byte(uint8_t b){ UART0->DATA = b; }
static inline uint8_t _rx_byte(void)  { return (uint8_t)(UART0->DATA & 0xFFu); }

/* ---- Stellaris UART (LM3S boards) ------------------------ */
#elif UART_DRIVER == UART_STELLARIS

void uart_init(void)
{
    /* 1. Gate clocks: UART0 + GPIOA */
    SYSCTL_RCGC1 |= SYSCTL_RCGC1_UART0EN;
    SYSCTL_RCGC2 |= SYSCTL_RCGC2_GPIOA;
    /* Small delay for clock to stabilise (>= 3 SYSCLK cycles) */
    __asm volatile("nop\nnop\nnop\nnop\n");

    /* 2. Enable GPIOA alternate function on PA0 (RX) and PA1 (TX) */
    GPIOA_AFSEL |= (1u << 0) | (1u << 1);

    /* 3. Disable UART before reconfiguring */
    UART0->CTL = 0;

    /* 4. Set baud-rate divisors (IBRD must be written before LCRH) */
    UART0->IBRD = UART_IBRD;
    UART0->FBRD = UART_FBRD;

    /* 5. 8N1, FIFO enabled
     *    LCRH must be written after IBRD/FBRD and causes them to latch */
    UART0->LCRH = UART_LCRH_WLEN8 | UART_LCRH_FEN;

    /* 6. Enable UART: TX, RX, UART_EN */
    UART0->CTL = UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE;
}

static inline int _tx_busy(void) { return UART0->FR & UART_FR_TXFF; }
static inline int _rx_ready(void){ return !(UART0->FR & UART_FR_RXFE); }
static inline void _tx_byte(uint8_t b){ UART0->DR = b; }
static inline uint8_t _rx_byte(void)  { return (uint8_t)(UART0->DR & 0xFFu); }

/* ---- STM32 USART (STM32VL board) ------------------------- */
#elif UART_DRIVER == UART_STM32

void uart_init(void)
{
    /* 1. Enable clocks: GPIOA + USART1 (APB2) */
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* 2. Configure PA9 (TX) = AF push-pull 50 MHz  → CRH[7:4]  = 0xB
     *                PA10 (RX) = floating input      → CRH[11:8] = 0x4 */
    uint32_t crh = GPIOA_CRH;
    crh &= ~(0xFFu << 4);          /* clear PA9 and PA10 fields */
    crh |=  (0xBu  << 4);          /* PA9: AF push-pull, 50 MHz */
    crh |=  (0x4u  << 8);          /* PA10: floating input      */
    GPIOA_CRH = crh;

    /* 3. Set baud rate */
    UART0->BRR = UART_BRR_VAL;

    /* 4. Enable USART: UE + TE + RE, 8N1 (default), no parity */
    UART0->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static inline int _tx_busy(void) { return !(UART0->SR & USART_SR_TXE);  }
static inline int _rx_ready(void){ return   UART0->SR & USART_SR_RXNE;  }
static inline void _tx_byte(uint8_t b){ UART0->DR = b; }
static inline uint8_t _rx_byte(void)  { return (uint8_t)(UART0->DR & 0xFFu); }

#else
#  error "UART_DRIVER not set — missing board selection?"
#endif /* UART_DRIVER */

/* ============================================================
 * ---- SECTION 2: common API (identical for all boards) -----
 * ============================================================ */

void uart_putc(char c)
{
    if (c == '\n') {
        while (_tx_busy()) {}
        _tx_byte('\r');
    }
    while (_tx_busy()) {}
    _tx_byte((uint8_t)c);
}

char uart_getc(void)
{
    while (!_rx_ready()) {}
    return (char)_rx_byte();
}

void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}

int uart_readline(char *buf, int maxlen)
{
    int  len = 0;
    char c;
    while (1) {
        c = uart_getc();
        if (c == '\r' || c == '\n') { uart_putc('\n'); break; }
        if ((c == '\b' || c == 0x7F) && len > 0) {
            uart_putc('\b'); uart_putc(' '); uart_putc('\b'); len--;
        } else if (c >= 0x20 && c < 0x7F && len < maxlen - 1) {
            buf[len++] = c; uart_putc(c);
        }
    }
    buf[len] = '\0';
    return len;
}

/* ============================================================
 * ---- SECTION 3: uart_vprintf / uart_printf ----------------
 * ============================================================ */

static void _rev(char *s, int n)
{
    int i=0,j=n-1; while(i<j){char t=s[i];s[i]=s[j];s[j]=t;i++;j--;}
}

static int _u32s(uint32_t v, char *b, int base, int up)
{
    const char *lo="0123456789abcdef", *hi="0123456789ABCDEF";
    const char *d=up?hi:lo; int n=0;
    if(!v){b[n++]='0';}else{while(v){b[n++]=d[v%base];v/=base;}}
    _rev(b,n); b[n]=0; return n;
}

static int _i32s(int32_t v, char *b)
{
    int n=0,neg=v<0;
    uint32_t u=neg?(uint32_t)(-(v+1))+1u:(uint32_t)v;
    if(!u){b[n++]='0';}else{while(u){b[n++]='0'+(u%10);u/=10;}}
    if(neg)b[n++]='-'; _rev(b,n); b[n]=0; return n;
}

static int _f64s(double val, char *buf, int prec)
{
    union{double d;uint64_t u;}uv; uv.d=val;
    uint64_t em=(uint64_t)0x7FFull<<52;
    if((uv.u&em)==em){
        if(uv.u&((1ULL<<52)-1)){strcpy(buf,"NaN");return 3;}
        if(uv.u>>63){strcpy(buf,"-Inf");return 4;}
        strcpy(buf,"Inf");return 3;
    }
    int neg=val<0.0; if(neg)val=-val;
    if(prec<0)prec=6; if(prec>10)prec=10;
    static const double p10[11]={1e0,1e1,1e2,1e3,1e4,1e5,1e6,1e7,1e8,1e9,1e10};
    val+=0.5/p10[prec];
    uint64_t ip=(uint64_t)val; double fr=val-(double)ip;
    char ib[24]; int il=0;
    if(!ip){ib[il++]='0';}else{uint64_t t=ip;while(t){ib[il++]='0'+(t%10);t/=10;}_rev(ib,il);}
    char fb[12]; int fl=0;
    for(int i=0;i<prec;i++){fr*=10.0;int dg=(int)fr;fb[fl++]='0'+dg;fr-=dg;}
    int pos=0;
    if(neg)buf[pos++]='-';
    for(int i=0;i<il;i++)buf[pos++]=ib[i];
    if(prec>0){buf[pos++]='.';for(int i=0;i<fl;i++)buf[pos++]=fb[i];}
    buf[pos]=0; return pos;
}

void uart_vprintf(const char *fmt, va_list ap)
{
    char buf[48];
    while(*fmt){
        if(*fmt!='%'){uart_putc(*fmt++);continue;}
        fmt++;
        int left=0; if(*fmt=='-'){left=1;fmt++;}
        int width=0; while(*fmt>='0'&&*fmt<='9') width=width*10+(*fmt++-'0');
        int prec=6;  if(*fmt=='.'){fmt++;prec=0;while(*fmt>='0'&&*fmt<='9') prec=prec*10+(*fmt++-'0');}
        while(*fmt=='l'||*fmt=='h') fmt++;
        char sp=*fmt++; const char *sptr=buf; int slen=0;
        switch(sp){
        case 'c': buf[0]=(char)va_arg(ap,int);buf[1]=0;slen=1;break;
        case 's': sptr=va_arg(ap,const char*);slen=(int)strlen(sptr);break;
        case 'd': slen=_i32s((int32_t)va_arg(ap,int),buf);break;
        case 'u': slen=_u32s((uint32_t)va_arg(ap,unsigned),buf,10,0);break;
        case 'x': slen=_u32s((uint32_t)va_arg(ap,unsigned),buf,16,0);break;
        case 'X': slen=_u32s((uint32_t)va_arg(ap,unsigned),buf,16,1);break;
        case 'p': buf[0]='0';buf[1]='x';
                  slen=_u32s((uint32_t)(uintptr_t)va_arg(ap,void*),buf+2,16,0)+2;break;
        case 'f': slen=_f64s(va_arg(ap,double),buf,prec);break;
        case '%': uart_putc('%');continue;
        default:  uart_putc('?');continue;
        }
        if(!left) for(int i=slen;i<width;i++) uart_putc(' ');
        for(int i=0;i<slen;i++) uart_putc(sptr[i]);
        if(left)  for(int i=slen;i<width;i++) uart_putc(' ');
    }
}

void uart_printf(const char *fmt, ...)
{
    va_list ap; va_start(ap,fmt); uart_vprintf(fmt,ap); va_end(ap);
}

/* ============================================================
 * ---- SECTION 4: Newlib syscall stubs ----------------------
 * ============================================================ */

int _write(int fd, const char *b, int n)
{ (void)fd; for(int i=0;i<n;i++) uart_putc(b[i]); return n; }

int _read(int fd, char *b, int n)
{ (void)fd; for(int i=0;i<n;i++) b[i]=uart_getc(); return n; }

int _close(int fd)                   { (void)fd; return -1; }
int _fstat(int fd, struct stat *st)  { (void)fd; st->st_mode=S_IFCHR; return 0; }
int _isatty(int fd)                  { (void)fd; return 1; }
int _lseek(int fd, int p, int d)     { (void)fd;(void)p;(void)d; return 0; }

extern uint8_t _sheap, _eheap;
void *_sbrk(int incr)
{
    static uint8_t *end = &_sheap;
    uint8_t *prev = end;
    if (end + incr > &_eheap) { errno = ENOMEM; return (void *)-1; }
    end += incr; return prev;
}

void __attribute__((noreturn)) _exit(int status)
{
    uart_printf("\n[HALT] _exit(%d)\n", status);
    __asm volatile("cpsid i\nbkpt 0\n_es: wfi\nb _es\n":::"memory");
    __builtin_unreachable();
}

int _kill(int pid, int sig)  { (void)pid; (void)sig; return -1; }
int _getpid(void)            { return 1; }
