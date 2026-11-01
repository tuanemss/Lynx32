/*
 *  Lynx32 - Dump menu
 *
 *  Copyright (c) 2015 xerub
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
#include "nand_llb.h"
#include "nand_fw.h"
#include "nor0.h"
#include "timer.h"
#include "blob.h"
#include "dump.h"

void list_available_targets(void)
{
    printf_("Available dump targets on this device:\n");
    printf_("  - nand_llb (or llb) : Low-Level Bootloader Img3 container (from Page 2)\n");
    printf_("  - llb_raw [pages]   : Full raw Boot Block from Page 0 (includes HW headers)\n");
    printf_("  - llb_info          : Display LLB Img3 container metadata\n");
    printf_("  - nand_fw (or fw)   : Full NAND Firmware partition (IMG2 + Img3 components)\n");
    printf_("  - nand_fw_info      : Display Image2 Superblock parameters\n");
    printf_("  - nor0              : SPI NOR Flash chip / block device\n");
    printf_("  - nor0_info         : Display SPI NOR Flash parameters\n");
    printf_("  - blob              : Onboard SHSH & APTicket blobs\n");
    printf_("  - mem <addr> <size> : Dump raw memory range\n");

    printf_("Detected block devices (can dump by name: dump <name>):\n");
    void *dev = bdev_stack;
    int count = 0;
    while (dev != NULL) {
        const char *name = bdev_name(dev);
        if (name && name[0] != '\0') {
            unsigned int sz = (version < 1940) ? 
                ((struct bdev6_t *)dev)->size_lo : 
                ((struct bdev_t *)dev)->size_lo;
            printf_("    * %-16s (size: 0x%x / %u bytes)\n", name, sz, sz);
            count++;
        }
        dev = *(void **)dev;
    }
    if (count == 0) {
        printf_("    (No block devices found in bdev_stack)\n");
    }
}

int do_dump_bdev(const char *target_name)
{
    void *dev = bdev_stack;
    while (dev != NULL) {
        const char *name = bdev_name(dev);
        if (name && strcmp(name, target_name) == 0) {
            break;
        }
        dev = *(void **)dev;
    }

    if (!dev) {
        printf_("[!] Block device '%s' not found on this device\n", target_name);
        return -1;
    }

    unsigned int sz = (version < 1940) ? 
        ((struct bdev6_t *)dev)->size_lo : 
        ((struct bdev_t *)dev)->size_lo;

    if (sz == 0) {
        printf_("[!] Block device '%s' reports size 0\n", target_name);
        return -1;
    }

    unsigned int t_start = timer_start();

    unsigned char *buf = malloc_(sz);
    if (!buf) {
        printf_("[!] Out of memory allocating %u bytes\n", sz);
        return -1;
    }

    int read_bytes = bdev_read(dev, buf, 0, sz);
    if (read_bytes <= 0) {
        printf_("[!] Error reading from %s\n", target_name);
        free_(buf);
        return -1;
    }

    get_addr = (unsigned int)buf;
    get_endp = get_addr + read_bytes;

    LOG_DEBUG("%s copied at %x - %x (%u bytes)\n", target_name, get_addr, get_endp, read_bytes);
    timer_print_elapsed(t_start);
    return 0;
}

int cmd_dump(int argc, CmdArg *argv)
{
    if (argc < 2) {
        printf_("usage: dump <target> [args]\n");
        list_available_targets();
        return 0;
    }

    const char *target = argv[1].string;

    if (strcmp(target, "help") == 0 || strcmp(target, "list") == 0) {
        list_available_targets();
        return 0;
    }

    if (strcmp(target, "nand_llb_raw") == 0 || strcmp(target, "llb_raw") == 0 || strcmp(target, "bootblock") == 0) {
        int custom_pages = 0;
        if (argc >= 3) {
            custom_pages = argv[2].integer;
        }
        return do_dump_nand_llb_raw(custom_pages);
    }

    if (strcmp(target, "nand_llb") == 0 || strcmp(target, "llb") == 0) {
        return do_dump_nand_llb();
    }

    if (strcmp(target, "llb_info") == 0) {
        return llb_info();
    }

    if (strcmp(target, "nand_fw") == 0 || strcmp(target, "fw") == 0) {
        return do_dump_nand_fw();
    }

    if (strcmp(target, "nand_fw_info") == 0 || strcmp(target, "fw_info") == 0) {
        return nand_fw_info();
    }

    if (strcmp(target, "nor0") == 0) {
        return do_dump_nor0();
    }

    if (strcmp(target, "nor0_info") == 0) {
        return nor0_info();
    }

    if (strcmp(target, "blob") == 0) {
        return save_all_blobs();
    }

    if (strcmp(target, "mem") == 0) {
        if (argc < 4) {
            printf_("Usage: dump mem <addr> <size>\n");
            return 0;
        }
        get_addr = argv[2].inthex;
        get_endp = get_addr + argv[3].inthex;
        LOG_DEBUG("Memory staged at %x - %x (%u bytes)\n", get_addr, get_endp, argv[3].inthex);
        printf_("dump success!\n");
        return 0;
    }

    /* Try to find as a generic block device by exact name (e.g. nor0, etc.) */
    return do_dump_bdev(target);
}
