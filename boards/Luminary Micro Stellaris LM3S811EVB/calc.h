/* ============================================================
 * calc.h  —  Calculator 1.0 expression engine
 *
 * Recursive-descent parser supporting:
 *   operators   + - * / ^  (with correct precedence)
 *   unary minus  -3  -(-4)
 *   parentheses  (2+3)*4
 *   functions    sqrt abs floor ceil round
 *                sin cos tan asin acos atan
 *                exp log log2 log10
 *   constants    pi  e  ans  inf  nan
 *   ANS          result of last successful evaluation
 *   history      ring buffer of last CALC_HIST_SIZE results
 *
 * Grammar (EBNF):
 *   expr    ::= term   (('+' | '-') term)*
 *   term    ::= power  (('*' | '/') power)*
 *   power   ::= unary  ('^' unary)*        — right-associative
 *   unary   ::= '-' unary | primary
 *   primary ::= number | '(' expr ')' | ident ('(' expr ')')?
 * ============================================================ */

#ifndef CALC_H
#define CALC_H

#include <stdint.h>

/* ---- Error codes ------------------------------------------ */
typedef enum {
    CALC_OK           = 0,
    CALC_ERR_SYNTAX   = 1,
    CALC_ERR_DIVZERO  = 2,
    CALC_ERR_PAREN    = 3,
    CALC_ERR_UNKNOWN  = 4,
    CALC_ERR_DOMAIN   = 5,
    CALC_ERR_OVERFLOW = 6,
} CalcErr;

extern const char * const CALC_ERR_STR[];

/* ---- History ring ----------------------------------------- */
#define CALC_HIST_SIZE  8

typedef struct {
    char   expr[128];
    double result;
    int    valid;
} CalcHistEntry;

/* ============================================================
 * API
 * ============================================================ */

/**
 * Evaluate expression string.
 * @param expr     NUL-terminated expression (read-only).
 * @param result   Filled on CALC_OK.
 * @param err_col  0-based column of error token, or -1.
 * @return CALC_OK or error code.
 */
CalcErr calc_eval(const char *expr, double *result, int *err_col);

void   calc_hist_add(const char *expr, double result);
void   calc_hist_print(void);
void   calc_hist_clear(void);
double calc_get_ans(void);

#endif /* CALC_H */
