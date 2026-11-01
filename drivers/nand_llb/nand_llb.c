/*
 *  Lynx32 - nand_llb dumper
 *
 *  Copyright (c) 2026 tuanem
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "plib.h"
#include "nand.h"
#include "timer.h"
#include "nand_llb.h"

int read_llb(h2fmi_read_boot_page_t read_page, int offset, int size, unsigned char *buf)
{
    int rv = 0;
    int page = div(offset, LLB_PAGE_SIZE);
    int start = offset - page * LLB_PAGE_SIZE;
    unsigned char *tmp = NULL;

    page += 2;
    if (start) {
        int chunk = LLB_PAGE_SIZE - start;
        if (!(tmp = malloc_(LLB_PAGE_SIZE))) return -1;
        rv = read_page(0, page, tmp);
        if (rv) {
            goto err;
        }
        memcpy(buf, tmp + start, chunk);
        buf += chunk;
        size -= chunk;
        page++;
    }

    for (; size >= LLB_PAGE_SIZE; page++) {
        rv = read_page(0, page, buf);
        if (rv) {
            goto err;
        }
        buf += LLB_PAGE_SIZE;
        size -= LLB_PAGE_SIZE;
    }

    if (size) {
        if (!tmp && !(tmp = malloc_(LLB_PAGE_SIZE))) return -1;
        rv = read_page(0, page, tmp);
        if (rv == 0) {
            memcpy(buf, tmp, size);
        }
    }

err:
    free_(tmp);
    return rv;
}

int read_nand_boot_pages(h2fmi_read_boot_page_t read_page, int start_page, int num_pages, unsigned char *buf)
{
    int rv = 0;
    for (int p = 0; p < num_pages; p++) {
        int page_idx = start_page + p;
        unsigned char *dst = buf + (p * LLB_PAGE_SIZE);
        rv = read_page(0, page_idx, dst);
        if (rv) {
            printf_("[!] Error reading boot page %d (rv=%d)\n", page_idx, rv);
            return rv;
        }
    }
    return 0;
}

int llb_info(void)
{
    h2fmi_t *p = h2fmi_select_();
    if (!p || !p->read_boot_page) {
        printf_("[!] h2fmi controller not available\n");
        return -1;
    }

    struct img3_header root;
    int rv = read_llb(p->read_boot_page, 0, sizeof(root), (void *)&root);
    if (rv) {
        printf_("[!] Failed to read LLB header (err: %d)\n", rv);
        return -1;
    }

    printf_("LLB: magic 0x%08x (%s) | size %u bytes | dataSize %u | shshOffset 0x%x\n",
            root.magic, (root.magic == 0x496d6733) ? "Img3" : "Unknown",
            root.size, root.dataSize, root.shshOffset);
    return 0;
}

int do_dump_nand_llb(void)
{
    int rv;
    struct img3_header root;
    h2fmi_t *p = h2fmi_select_();

    if (!p || !p->read_boot_page) {
        printf_("[!] h2fmi controller not available\n");
        return -1;
    }

    LOG_DEBUG("Reading LLB header from NAND boot page 0...\n");
    unsigned int t_start = timer_start();
    rv = read_llb(p->read_boot_page, 0, sizeof(root), (void *)&root);
    if (rv) {
        printf_("[!] Failed to read LLB header (err: %d)\n", rv);
        return -1;
    }

    /* Img3 magic is 0x496d6733 ("Img3") */
    if (root.magic != 0x496d6733 && root.magic != 0x33676d49) {
        LOG_DEBUG("Notice: LLB magic is 0x%08x\n", root.magic);
    }

    if (root.size == 0 || root.size > 0x100000) {
        printf_("[!] Suspicious LLB size: %u bytes\n", root.size);
        return -1;
    }

    LOG_DEBUG("Found LLB Img3 container: %u bytes (dataSize: %u)\n", root.size, root.dataSize);

    unsigned char *llb_buf = malloc_(root.size);
    if (!llb_buf) {
        printf_("[!] Out of memory allocating %u bytes for LLB\n", root.size);
        return -1;
    }

    LOG_DEBUG("Reading %u bytes of raw LLB...\n", root.size);
    rv = read_llb(p->read_boot_page, 0, root.size, llb_buf);
    if (rv) {
        printf_("[!] Error reading full LLB data\n");
        free_(llb_buf);
        return -1;
    }

    get_addr = (unsigned int)llb_buf;
    get_endp = get_addr + root.size;

    LOG_DEBUG("nand_llb copied at %x - %x (%u bytes)\n", get_addr, get_endp, root.size);
    timer_print_elapsed(t_start);
    return 0;
}

int do_dump_nand_llb_raw(int num_pages)
{
    h2fmi_t *p = h2fmi_select_();
    if (!p || !p->read_boot_page) {
        printf_("[!] h2fmi controller not available\n");
        return -1;
    }

    if (num_pages <= 0) {
        struct img3_header root;
        if (read_llb(p->read_boot_page, 0, sizeof(root), (void *)&root) == 0 &&
            root.size > 0 && root.size < 0x200000) {
            num_pages = 2 + (root.size + LLB_PAGE_SIZE - 1) / LLB_PAGE_SIZE;
        } else {
            num_pages = 128;
        }
    }

    unsigned int total_size = num_pages * LLB_PAGE_SIZE;
    unsigned int t_start = timer_start();

    unsigned char *raw_buf = malloc_(total_size);
    if (!raw_buf) {
        printf_("[!] Out of memory allocating %u bytes for raw boot pages\n", total_size);
        return -1;
    }

    int rv = read_nand_boot_pages(p->read_boot_page, 0, num_pages, raw_buf);
    if (rv) {
        printf_("[!] Failed to read raw boot pages (err: %d)\n", rv);
        free_(raw_buf);
        return -1;
    }

    get_addr = (unsigned int)raw_buf;
    get_endp = get_addr + total_size;

    LOG_DEBUG("nand_llb_raw (%d pages) copied at %x - %x (%u bytes)\n", num_pages, get_addr, get_endp, total_size);
    timer_print_elapsed(t_start);
    return 0;
}
