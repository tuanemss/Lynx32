#ifndef NAND_LLB_H_INCLUDED
#define NAND_LLB_H_INCLUDED

#include "plib.h"
#include "nand.h"

#define LLB_PAGE_SIZE 1536

#ifndef IMG3_ROOT_DEFINED
#define IMG3_ROOT_DEFINED
struct img3_header {
    unsigned int magic;
    unsigned int size;
    unsigned int dataSize;
    unsigned int shshOffset;
    unsigned int name;
};
#endif

int do_dump_nand_llb(void);
int do_dump_nand_llb_raw(int num_pages);
int llb_info(void);
int read_llb(h2fmi_read_boot_page_t read_page, int offset, int size, unsigned char *buf);
int read_nand_boot_pages(h2fmi_read_boot_page_t read_page, int start_page, int num_pages, unsigned char *buf);

#endif /* NAND_LLB_H_INCLUDED */
