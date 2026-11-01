/*
 *  ibex - pseudo-library
 *
 *  Division algorithm from Wikipedia
 */


#include "plib.h"


unsigned
div(unsigned N, unsigned D)
{
    int i;
    unsigned Q = 0;
    unsigned R = 0;
    if (D == 0) {
        return -1; /* DivisionByZeroException */
    }
    for (i = sizeof(N) * 8 - 1; i >= 0; i--) {
        R <<= 1;
        R = (R & ~1) | ((N >> i) & 1);
        if (R >= D) {
            R -= D;
            Q |= (1 << i);
        }
    }
    return Q;
}

unsigned
__aeabi_uidiv(unsigned N, unsigned D)
{
    return div(N, D);
}

__attribute__((naked)) unsigned
__aeabi_uidivmod(unsigned N, unsigned D)
{
    (void)N;
    (void)D;
    asm volatile(
        "push {r4, r5, lr}\n"
        "mov r4, r0\n"
        "mov r5, r1\n"
        "bl div\n"
        "mul r5, r0\n"
        "sub r1, r4, r5\n"
        "pop {r4, r5, pc}\n"
    );
}
