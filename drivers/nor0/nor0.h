#ifndef NOR0_H_INCLUDED
#define NOR0_H_INCLUDED

#include "plib.h"
#include "nand.h"

void *find_nor0_bdev(void);
int do_dump_nor0(void);
int nor0_info(void);

#endif /* NOR0_H_INCLUDED */
