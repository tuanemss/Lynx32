#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#include "plib.h"

/*
 * High-resolution cycle timer using ARM Cortex-A8 (Apple A4 / S5L8930X)
 * Performance Monitoring Unit (PMU) Cycle Counter (PMCCNTR).
 */
static inline unsigned int timer_start(void)
{
    unsigned int val;
    /*
     * CP15 c9 PMCR:
     * Bit 0: Enable all counters (E)
     * Bit 2: Reset cycle counter (C)
     * Bit 3: Cycle count divider (D=1, counts every 64 cycles)
     * Value: 0x0d
     */
    asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0x0d));

    /* CP15 c9 PMCNTENSET: Enable PMCCNTR (Bit 31) */
    asm volatile("mcr p15, 0, %0, c9, c12, 1" :: "r"(0x80000000));

    /* Read starting cycle count */
    asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(val));
    return val;
}

static inline unsigned int timer_elapsed_centis(unsigned int t_start)
{
    unsigned int t_end;
    asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(t_end));
    return div(t_end - t_start, 156250);
}

static inline void timer_print_elapsed(unsigned int t_start)
{
    unsigned int centis = timer_elapsed_centis(t_start);
    unsigned int sec = div(centis, 100);
    unsigned int cs = centis - (sec * 100);
    printf_("dump success!\n");
    printf_("took - %u.%02us\n", sec, cs);
}

#endif /* TIMER_H_INCLUDED */
