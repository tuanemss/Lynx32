#ifndef DUMP_H_included
#define DUMP_H_included

#include "plib.h"
#include "nand_fw.h"

int cmd_dump(int argc, CmdArg *argv);
int do_dump_nand_llb(void);
int do_dump_nand_llb_raw(int num_pages);
int do_dump_bdev(const char *target_name);
void list_available_targets(void);

#endif
