/* ============================================================
 * calc.c  —  Recursive-descent expression evaluator
 * ============================================================ */

#define _GNU_SOURCE   /* expose M_PI, M_E, M_LOG2E in newlib/glibc */

#include "calc.h"
#include "uart.h"

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* Fallback constants if the toolchain doesn't expose them */
#ifndef M_PI
#  define M_PI    3.14159265358979323846
#endif
#ifndef M_E
#  define M_E     2.71828182845904523536
#endif
#ifndef M_LOG2E
#  define M_LOG2E 1.44269504088896340736
#endif

/* ---- Error string table ----------------------------------- */
const char * const CALC_ERR_STR[] = {
    [CALC_OK]           = "OK",
    [CALC_ERR_SYNTAX]   = "Syntax error",
    [CALC_ERR_DIVZERO]  = "Division by zero",
    [CALC_ERR_PAREN]    = "Mismatched parentheses",
    [CALC_ERR_UNKNOWN]  = "Unknown identifier",
    [CALC_ERR_DOMAIN]   = "Math domain error",
    [CALC_ERR_OVERFLOW] = "Overflow",
};

/* ============================================================
 * History ring
 * ============================================================ */
static CalcHistEntry s_hist[CALC_HIST_SIZE];
static int           s_head  = 0;
static int           s_count = 0;
static double        s_ans   = 0.0;

void calc_hist_add(const char *expr, double result)
{
    CalcHistEntry *e = &s_hist[s_head];
    strncpy(e->expr, expr, sizeof(e->expr)-1);
    e->expr[sizeof(e->expr)-1] = '\0';
    e->result = result; e->valid = 1;
    s_head = (s_head + 1) % CALC_HIST_SIZE;
    if (s_count < CALC_HIST_SIZE) s_count++;
    s_ans = result;
}

void calc_hist_clear(void)
{
    memset(s_hist, 0, sizeof(s_hist));
    s_head = s_count = 0;
}

double calc_get_ans(void) { return s_ans; }

void calc_hist_print(void)
{
    if (!s_count) { uart_puts("  (no history)\n"); return; }
    int start = (s_head - s_count + CALC_HIST_SIZE) % CALC_HIST_SIZE;
    uart_printf("\n  %-3s  %-36s  %s\n","No.","Expression","Result");
    uart_puts  ("  -----------------------------------------------\n");
    for (int i=0;i<s_count;i++) {
        int idx = (start+i)%CALC_HIST_SIZE;
        uart_printf("  %-3d  %-36s  %.10g\n",
                    i+1, s_hist[idx].expr, s_hist[idx].result);
    }
    uart_putc('\n');
}

/* ============================================================
 * Tokeniser
 * ============================================================ */
typedef enum {
    TOK_NUM, TOK_IDENT, TOK_PLUS, TOK_MINUS, TOK_STAR,
    TOK_SLASH, TOK_CARET, TOK_LPAREN, TOK_RPAREN,
    TOK_END, TOK_ERR,
} TokType;

typedef struct {
    TokType type; double num; char ident[32]; int col;
} Token;

typedef struct {
    const char *src; const char *p;
    Token cur; CalcErr err; int err_col;
} Parser;

static void _next(Parser *ps)
{
    const char *p = ps->p;
    while (*p && isspace((unsigned char)*p)) p++;
    Token t; t.col = (int)(p - ps->src);
    if (!*p) { t.type=TOK_END; ps->cur=t; ps->p=p; return; }

    if (isdigit((unsigned char)*p) ||
        (*p=='.' && isdigit((unsigned char)*(p+1)))) {
        char nb[48]; int nl=0;
        while (isdigit((unsigned char)*p) && nl<47) nb[nl++]=*p++;
        if (*p=='.' && nl<47) { nb[nl++]=*p++; while(isdigit((unsigned char)*p)&&nl<47) nb[nl++]=*p++; }
        if ((*p=='e'||*p=='E')&&nl<47) { nb[nl++]=*p++;
            if ((*p=='+'||*p=='-')&&nl<47) nb[nl++]=*p++;
            while(isdigit((unsigned char)*p)&&nl<47) nb[nl++]=*p++; }
        nb[nl]='\0'; t.type=TOK_NUM;
        extern double strtod(const char*,char**);
        char *end; t.num=strtod(nb,&end);
        ps->cur=t; ps->p=p; return;
    }

    if (isalpha((unsigned char)*p)||*p=='_') {
        int il=0;
        while ((isalnum((unsigned char)*p)||*p=='_')&&il<31) t.ident[il++]=*p++;
        t.ident[il]='\0'; t.type=TOK_IDENT;
        ps->cur=t; ps->p=p; return;
    }

    switch (*p) {
    case '+': t.type=TOK_PLUS;   break; case '-': t.type=TOK_MINUS;  break;
    case '*': t.type=TOK_STAR;   break; case '/': t.type=TOK_SLASH;  break;
    case '^': t.type=TOK_CARET;  break; case '(': t.type=TOK_LPAREN; break;
    case ')': t.type=TOK_RPAREN; break; default:  t.type=TOK_ERR;    break;
    }
    p++; ps->cur=t; ps->p=p;
}

static void _err(Parser *ps, CalcErr e)
{
    if (!ps->err) { ps->err=e; ps->err_col=ps->cur.col; }
}

static double _expr(Parser *ps);

static double _primary(Parser *ps)
{
    Token t = ps->cur;
    if (t.type==TOK_NUM)    { _next(ps); return t.num; }
    if (t.type==TOK_LPAREN) {
        _next(ps); double v=_expr(ps);
        if (ps->cur.type!=TOK_RPAREN) { _err(ps,CALC_ERR_PAREN); return 0; }
        _next(ps); return v;
    }
    if (t.type==TOK_IDENT) {
        _next(ps);
        if (!strcmp(t.ident,"pi"))  return M_PI;
        if (!strcmp(t.ident,"e"))   return M_E;
        if (!strcmp(t.ident,"ans")) return s_ans;
        if (!strcmp(t.ident,"inf")) return (double)1.0/0.0;
        if (!strcmp(t.ident,"nan")) return 0.0/0.0;

        if (ps->cur.type!=TOK_LPAREN) { _err(ps,CALC_ERR_UNKNOWN); return 0; }
        _next(ps); double a=_expr(ps);
        if (ps->cur.type!=TOK_RPAREN) { _err(ps,CALC_ERR_PAREN); return 0; }
        _next(ps);

#define FNCHECK(nm,fn) if(!strcmp(t.ident,nm)) return fn(a)
        FNCHECK("abs",fabs); FNCHECK("floor",floor); FNCHECK("ceil",ceil);
        FNCHECK("round",round); FNCHECK("sin",sin); FNCHECK("cos",cos);
        FNCHECK("tan",tan); FNCHECK("atan",atan); FNCHECK("exp",exp);
        if (!strcmp(t.ident,"sqrt")) {
            if (a<0.0){_err(ps,CALC_ERR_DOMAIN);return 0;} return sqrt(a); }
        if (!strcmp(t.ident,"asin")) {
            if (a<-1||a>1){_err(ps,CALC_ERR_DOMAIN);return 0;} return asin(a);}
        if (!strcmp(t.ident,"acos")) {
            if (a<-1||a>1){_err(ps,CALC_ERR_DOMAIN);return 0;} return acos(a);}
        if (!strcmp(t.ident,"log"))   { if(a<=0){_err(ps,CALC_ERR_DOMAIN);return 0;} return log(a); }
        if (!strcmp(t.ident,"log2"))  { if(a<=0){_err(ps,CALC_ERR_DOMAIN);return 0;} return log2(a); }
        if (!strcmp(t.ident,"log10")) { if(a<=0){_err(ps,CALC_ERR_DOMAIN);return 0;} return log10(a); }
        _err(ps,CALC_ERR_UNKNOWN); return 0;
    }
    _err(ps,CALC_ERR_SYNTAX); return 0;
}

static double _unary(Parser *ps)
{
    if (ps->cur.type==TOK_MINUS) { _next(ps); return -_unary(ps); }
    if (ps->cur.type==TOK_PLUS)  { _next(ps); return  _unary(ps); }
    return _primary(ps);
}

static double _power(Parser *ps)
{
    double b=_unary(ps);
    if (ps->cur.type==TOK_CARET) { _next(ps); return pow(b,_power(ps)); }
    return b;
}

static double _term(Parser *ps)
{
    double v=_power(ps);
    while (!ps->err && (ps->cur.type==TOK_STAR||ps->cur.type==TOK_SLASH)) {
        TokType op=ps->cur.type; _next(ps); double r=_power(ps);
        if (op==TOK_STAR) v*=r;
        else { if(!r){_err(ps,CALC_ERR_DIVZERO);return 0;} v/=r; }
    }
    return v;
}

static double _expr(Parser *ps)
{
    double v=_term(ps);
    while (!ps->err && (ps->cur.type==TOK_PLUS||ps->cur.type==TOK_MINUS)) {
        TokType op=ps->cur.type; _next(ps); double r=_term(ps);
        if (op==TOK_PLUS) v+=r; else v-=r;
    }
    return v;
}

/* ============================================================
 * Public: calc_eval
 * ============================================================ */
CalcErr calc_eval(const char *expr, double *result, int *err_col)
{
    const char *p=expr;
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) { if(err_col)*err_col=0; return CALC_ERR_SYNTAX; }

    Parser ps; memset(&ps,0,sizeof(ps));
    ps.src=expr; ps.p=expr;
    _next(&ps);
    double val=_expr(&ps);
    if (!ps.err && ps.cur.type!=TOK_END) _err(&ps,CALC_ERR_SYNTAX);
    if (err_col) *err_col=ps.err_col;
    if (ps.err)  { *result=0.0; return ps.err; }
    *result=val; return CALC_OK;
}
