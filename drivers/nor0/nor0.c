/*
 *  Lynx32 - nor0 dumper
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
#include "nor0.h"
#include "timer.h"
#include "apple/Image2Format.h"

void *find_nor0_bdev(void)
{
    void *dev = bdev_stack;
    while (dev != NULL) {
        const char *name = bdev_name(dev);
        if (name && strcmp(name, "nor0") == 0) {
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

int nor0_info(void)
{
    void *dev = find_nor0_bdev();
    if (!dev) {
        printf_("[!] Block device 'nor0' not found on this device\n");
        printf_("    (iPhone 4 stores boot blocks in NAND. 'nor0' is used on SPI NOR devices like 3GS/iPod or with NOR bdev registered)\n");
        return -1;
    }

    unsigned int sz = get_bdev_size(dev);
    (void)sz;
    LOG_DEBUG("nor0: size 0x%x (%u bytes / %u KB)\n", sz, sz, sz / 1024);

    Image2Superblock sb;
    int read_bytes = bdev_read(dev, &sb, 0, sizeof(sb));
    if (read_bytes >= (int)sizeof(sb) && sb.isMagic == kImage2SuperMagic) {
        LOG_DEBUG("  Superblock: IMG2 | Granule: %u bytes | Image Avail: %u\n",
                (unsigned int)sb.isImageGranule, (unsigned int)sb.isImageAvail);
    }
    return 0;
}

int do_dump_nor0(void)
{
    void *dev = find_nor0_bdev();
    if (!dev) {
        printf_("[!] Block device 'nor0' not found on this device\n");
        printf_("    (Note: On iPhone 4, bootloaders reside in NAND boot pages. Use 'dump nand_llb' or 'dump nand_fw')\n");
        return -1;
    }

    unsigned int sz = get_bdev_size(dev);
    if (sz == 0) {
        printf_("[!] 'nor0' reports size 0\n");
        return -1;
    }

    unsigned int t_start = timer_start();

    unsigned char *buf = malloc_(sz);
    if (!buf) {
        printf_("[!] Out of memory allocating %u bytes for nor0\n", sz);
        return -1;
    }

    int read_bytes = bdev_read(dev, buf, 0, sz);
    if (read_bytes <= 0) {
        printf_("[!] Error reading from nor0\n");
        free_(buf);
        return -1;
    }

    get_addr = (unsigned int)buf;
    get_endp = get_addr + read_bytes;

    LOG_DEBUG("nor0 copied at %x - %x (%u bytes)\n", get_addr, get_endp, read_bytes);
    timer_print_elapsed(t_start);
    return 0;
}
