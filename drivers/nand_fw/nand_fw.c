/*
 *  Lynx32 - nand_firmware dumper
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
#include "nand_fw.h"
#include "timer.h"
#include "apple/Image2Format.h"

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

static void *find_nand_fw_bdev(void)
{
    void *dev = bdev_stack;
    while (dev != NULL) {
        const char *name = bdev_name(dev);
        if (name && strcmp(name, "nand_firmware") == 0) {
            return dev;
        }
        dev = *(void **)dev;
    }
    return NULL;
}

static unsigned int get_bdev_size(void *dev)
{
    if (!dev) return 0;
    if (version < 1940) {
        return ((struct bdev6_t *)dev)->size_lo;
    } else {
        return ((struct bdev_t *)dev)->size_lo;
    }
}

static void __attribute__((unused)) print_img3_name(unsigned int name)
{
    char tag[5];
    tag[0] = (char)(name & 0xFF);
    tag[1] = (char)((name >> 8) & 0xFF);
    tag[2] = (char)((name >> 16) & 0xFF);
    tag[3] = (char)((name >> 24) & 0xFF);
    tag[4] = '\0';
    printf_("%s", tag);
}

int nand_fw_info(void)
{
    void *dev = find_nand_fw_bdev();
    if (!dev) {
        printf_("[!] Block device 'nand_firmware' not found\n");
        return -1;
    }

    unsigned int sz = get_bdev_size(dev);
    (void)sz;
    LOG_DEBUG("nand_firmware: size 0x%x (%u bytes / %u KB)\n", sz, sz, sz / 1024);

    Image2Superblock sb;
    int read_bytes = bdev_read(dev, &sb, 0, sizeof(sb));
    if (read_bytes < (int)sizeof(sb)) {
        printf_("[!] Failed to read Image2 Superblock\n");
        return -1;
    }

    if (sb.isMagic == kImage2SuperMagic) {
        LOG_DEBUG("  Superblock: IMG2 | Granule: %u | ImgOffset: 0x%x | BootBlock: %u | Avail: %u\n",
                (unsigned int)sb.isImageGranule,
                (unsigned int)(sb.isImageOffset * sb.isImageGranule),
                (unsigned int)(sb.isBootBlockSize * sb.isImageGranule),
                (unsigned int)sb.isImageAvail);
    } else {
        printf_("  Superblock Magic: 0x%08x\n", (unsigned int)sb.isMagic);
    }
    return 0;
}

int do_dump_nand_fw(void)
{
    void *dev = find_nand_fw_bdev();
    if (!dev) {
        printf_("[!] Block device 'nand_firmware' not found\n");
        return -1;
    }

    unsigned int sz = get_bdev_size(dev);
    if (sz == 0) {
        printf_("[!] 'nand_firmware' reports size 0\n");
        return -1;
    }

    LOG_DEBUG("Dumping 'nand_firmware' (size: 0x%x / %u bytes)...\n", sz, sz);
    unsigned int t_start = timer_start();

    unsigned char *buf = malloc_(sz);
    if (!buf) {
        printf_("[!] Out of memory allocating %u bytes for firmware\n", sz);
        return -1;
    }

    int read_bytes = bdev_read(dev, buf, 0, sz);
    if (read_bytes <= 0) {
        printf_("[!] Error reading from nand_firmware\n");
        free_(buf);
        return -1;
    }

    /* Validate Superblock */
    Image2Superblock *sb = (Image2Superblock *)buf;
    if (sb->isMagic == kImage2SuperMagic) {
        LOG_DEBUG("Valid Image2 Superblock detected (Granule: %u bytes)\n", (unsigned int)sb->isImageGranule);
    }

    /* Scan for embedded Img3 containers */
    LOG_DEBUG("Scanning embedded Img3 components...\n");
    int comp_count = 0;
    for (unsigned int off = 0; off + sizeof(struct img3_header) <= (unsigned int)read_bytes; off += 512) {
        struct img3_header *h = (struct img3_header *)(buf + off);
        if (h->magic == 0x496d6733) {
#if DEBUG_LOG
            printf_("[DEBUG]   [%2d] Offset 0x%06x | Tag: ", comp_count, off);
            print_img3_name(h->name);
            printf_(" | Size: %7u bytes\n", h->size);
#endif
            comp_count++;
        }
    }
    if (comp_count > 0) {
        LOG_DEBUG("Discovered %d Img3 firmware components!\n", comp_count);
    }

    get_addr = (unsigned int)buf;
    get_endp = get_addr + read_bytes;

    LOG_DEBUG("nand_firmware (%d components) copied at %x - %x (%u bytes)\n", comp_count, get_addr, get_endp, read_bytes);
    timer_print_elapsed(t_start);
    return 0;
}
