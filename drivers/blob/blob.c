/*
 *  ibex - blob stuff
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
#include "nor0.h"
#include "timer.h"
#include "blob.h"
#include "fb.h"
#include "device.h"

struct img3_root {
    unsigned int magic;
    unsigned int size;
    unsigned int dataSize;
    unsigned int shshOffset;
    unsigned int name;
};

int
decrypt_shsh(void *rsa)
{
    static const unsigned char shshkey[] = { 0xDB, 0x1F, 0x5B, 0x33, 0x60, 0x6C, 0x5F, 0x1C, 0x19, 0x34, 0xAA, 0x66, 0x58, 0x9C, 0x06, 0x61 };
    static unsigned char shshbuf[sizeof(shshkey)];
    static int beentheredonethat = 0;
    int rv;
    if (!beentheredonethat) {
        rv = aes_crypto_cmd_(AES_ENCRYPT, (char *)shshkey, shshbuf, KBAG_IV_SIZE, SHSH_KEY, NULL, NULL);
        if (rv) {
            return -1;
        }
        beentheredonethat = 1;
    }
    rv = aes_crypto_cmd_(AES_DECRYPT, rsa, rsa, 0x80, 0, (char *)shshbuf, NULL);
    if (rv) {
        return -1;
    }
    return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmultichar"

static struct blob_t *
save_llb_from_nor0(void *nor0)
{
    int rv;
    struct blob_t *llb;
    struct img3_root root;
    unsigned int *shsh;
    unsigned int sz;

    /* On SPI NOR flash (iPad 1, etc.), LLB starts at offset 0x8000 (after 32KB boot block) */
    rv = bdev_read(nor0, &root, 0x8000, sizeof(root));
    if (rv != (int)sizeof(root)) {
        printf_("nor0 llb read err1\n");
        return NULL;
    }

    if (!root.shshOffset || root.shshOffset >= root.dataSize) {
        printf_("nor0 bad shsh offset\n");
        return NULL;
    }

    sz = root.dataSize - root.shshOffset + 64;
    llb = malloc_(offsetof(struct blob_t, data) + sz);
    if (!llb) {
        printf_("out of memory\n");
        return NULL;
    }

    rv = bdev_read(nor0, llb->data, 0x8000 + root.shshOffset + 20 - 64, sz);
    if (rv != (int)sz) {
        printf_("nor0 llb read err2\n");
        free_(llb);
        return NULL;
    }

    shsh = (unsigned int *)(llb->data + 64);
    if (*shsh != 'SHSH') {
        printf_("bad signature\n");
        free_(llb);
        return NULL;
    }

    rv = decrypt_shsh(shsh + 3);
    if (rv) {
        printf_("aes err\n");
        free_(llb);
        return NULL;
    }

    device_feed_llb_data(llb->data, sz);

    llb->type = 'illb';
    llb->size = sz;
    return llb;
}

static struct blob_t *
save_llb(void)
{
    /* Auto-detect if device has SPI NOR ('nor0') such as iPad 1, iPod touch 3G, iPhone 3GS */
    void *nor0 = find_nor0_bdev();
    if (nor0) {
        return save_llb_from_nor0(nor0);
    }

    /* Otherwise iPhone 4 (bootloader in NAND boot pages via h2fmi) */
    int rv;
    struct blob_t *llb;
    struct img3_root root;
    unsigned int *shsh;
    unsigned int sz;
    h2fmi_t *p = h2fmi_select_();

    if (!p || !p->read_boot_page) {
        printf_("h2fmi err\n");
        return NULL;
    }
    rv = read_llb(p->read_boot_page, 0, sizeof(root), (void *)&root);
    if (rv) {
        printf_("llb read err1\n");
        return NULL;
    }

    if (!root.shshOffset || root.shshOffset >= root.dataSize) {
        printf_("bad shsh offset\n");
        return NULL;
    }
    sz = root.dataSize - root.shshOffset + 64;
    llb = malloc_(offsetof(struct blob_t, data) + sz);
    if (!llb) {
        printf_("out of memory\n");
        return NULL;
    }
    rv = read_llb(p->read_boot_page, root.shshOffset + 20 - 64, sz, llb->data);
    if (rv) {
        printf_("llb read err2\n");
        free_(llb);
        return NULL;
    }
    shsh = (unsigned int *)(llb->data + 64);
    if (*shsh != 'SHSH') {
        printf_("bad signature\n");
        free_(llb);
        return NULL;
    }

    rv = decrypt_shsh(shsh + 3);
    if (rv) {
        printf_("aes err\n");
        free_(llb);
        return NULL;
    }

    device_feed_llb_data(llb->data, sz);

    llb->type = 'illb';
    llb->size = sz;
    return llb;
}

static struct blob_t *
save_krnl(void)
{
    struct blob_t *blob;
    struct img3_root *root;
    unsigned int size;
    char *addr = (char *)TARGET_LOADADDR + 0x1000000;

    /* load kernel */
    ensure_boot_mounted();
    if ((size = 0x3000000, fs_loadfile_("/boot/System/Library/Caches/com.apple.kernelcaches/kernelcache", addr, &size)) &&
        (size = 0x3000000, fs_loadfile_("/boot/System/Library/Caches/com.apple.kernelcaches/kernelcache.s5l8920x", addr, &size)) &&
        (size = 0x3000000, fs_loadfile_("/boot/System/Library/Caches/com.apple.kernelcaches/kernelcache.s5l8922x", addr, &size)) &&
        (size = 0x3000000, fs_loadfile_("/boot/System/Library/Caches/com.apple.kernelcaches/kernelcache.s5l8720x", addr, &size))) {
        printf_("kernel read err1\n");
        return NULL;
    }

    root = (struct img3_root *)addr;
    if (!root->shshOffset || root->shshOffset >= root->dataSize) {
        return NULL;
    }
    blob = malloc_(sizeof(struct blob_t));
    if (!blob) {
        printf_("out of memory\n");
        return NULL;
    }
    if (root->shshOffset + 20 + 12 + 0x80 > size) {
        printf_("kernel read err2\n");
        free_(blob);
        return NULL;
    }
    memcpy(blob->data, addr + root->shshOffset + 20 + 12, 0x80);
    /*if (decrypt_shsh(blob->data)) {
        printf_("aes err\n");
        free_(blob);
        return NULL;
    }*/
    blob->type = 'krnl';
    blob->size = 0x80;
    return blob;
}

static struct blob_t *
save_ticket(struct firmware_image *e)
{
    struct blob_t *ticket;
    unsigned int tmp[16];
    unsigned int result;
    result = bdev_read(e->bdev, tmp, e->offset_lo, sizeof(tmp));
    if (result != sizeof(tmp)) {
        printf_("ticket read err1\n");
        return NULL;
    }
    ticket = malloc_(offsetof(struct blob_t, data) + tmp[15]);
    if (!ticket) {
        printf_("out of memory\n");
        return NULL;
    }
    result = bdev_read(e->bdev, ticket->data, e->offset_lo + sizeof(tmp), tmp[15]);
    if (result != tmp[15]) {
        free_(ticket);
        printf_("ticket read err2\n");
        return NULL;
    }
    ticket->type = e->info.type;
    ticket->size = result;
    return ticket;
}

static struct blob_t *
save_blob(struct firmware_image *e)
{
    struct blob_t *blob;
    struct img3_root root;
    unsigned int result;
    if (e->info.type == 'SCAB') {
        return save_ticket(e);
    }
    result = bdev_read(e->bdev, &root, e->offset_lo, sizeof(root));
    if (result != sizeof(root)) {
        printf_("image read err1\n");
        return NULL;
    }
    if (!root.shshOffset || root.shshOffset >= root.dataSize) {
        return NULL;
    }
    blob = malloc_(sizeof(struct blob_t));
    if (!blob) {
        printf_("out of memory\n");
        return NULL;
    }
    result = bdev_read(e->bdev, blob->data, e->offset_lo + root.shshOffset + 20 + 12, 0x80);
    if (result != 0x80) {
        free_(blob);
        printf_("image read err2\n");
        return NULL;
    }
    if (decrypt_shsh(blob->data)) {
        free_(blob);
        printf_("aes err\n");
        return NULL;
    }
    blob->type = e->info.type;
    blob->size = 0x80;
    return blob;
}

int
save_all_blobs(void)
{
    struct device_info dev;
    char line_buf[80];
    unsigned int t_start;
    unsigned i, n = 0;
    unsigned int total;
    unsigned char *data;
    struct blob_t *blobs[24];
    struct firmware_image *e;

    t_start = timer_start();

    /* Low-Level Bootloader (LLB) */
    blobs[n++] = save_llb();

    /* Detect device info (Model, iOS version, ECID, Board) */
    device_detect(&dev);

    fb_init_console(NULL, 0);

    /* Print Thanks & Credits */
    fb_print_line("=========================", 0xFFFFFF);
    fb_print_line(": Lynx32 - make by @tuanemss", 0xFFFFFF);
    fb_print_line(":", 0xFFFFFF);
    fb_print_line(": copyright (c) 2023 - 2026", 0xFFFFFF);
    fb_print_line(":", 0xFFFFFF);
    fb_print_line(": BUILD_TAG: 2026.1.10", 0xFFFFFF);
    fb_print_line(":", 0xFFFFFF);
    fb_print_line(": BUILD_STYLE: RELEASE", 0xFFFFFF);
    fb_print_line(":", 0xFFFFFF);
    fb_print_line(": credit & thanks to: xerub,", 0xFFFFFF);
    fb_print_line(": iH8sn0w, Chronic-Dev.", 0xFFFFFF);
    fb_print_line(":", 0xFFFFFF);
    fb_print_line("=========================", 0xFFFFFF);

    /* Print Device, iOS, and ECID */
    snprintf_(line_buf, sizeof(line_buf), "iDevice: %s", dev.model);
    fb_print_line(line_buf, 0xFFFFFF);

    snprintf_(line_buf, sizeof(line_buf), "iOS: %s", dev.ios_version);
    fb_print_line(line_buf, 0xFFFFFF);

    snprintf_(line_buf, sizeof(line_buf), "ECID: %s", dev.ecid);
    fb_print_line(line_buf, 0xFFFFFF);

    fb_print_line("=========================", 0xFFFFFF);

    if (blobs[0]) {
        fb_print_line("Dumped: LLB SHSH", 0xFFFFFF);
    }

    /* Embedded Firmware Images (iBoot, DeviceTree, Logo, RecMode, etc.) */
    for (e = ((struct firmware_image *)image_list)->next; n < ARRAY_SIZE(blobs) - 1 && e != image_list; e = e->next) {
        if (e->info.type == 'illb') {
            continue;
        }
        blobs[n++] = save_blob(e);
        if (blobs[n - 1]) {
            const char *name = NULL;
            if (e->info.type == 'ibot') name = "iBoot";
            else if (e->info.type == 'dtre') name = "DeviceTree";
            else if (e->info.type == 'logo') name = "AppleLogo";
            else if (e->info.type == 'recm') name = "Recovery Logo";
            else if (e->info.type == 'bat0') name = "Battery Low 0";
            else if (e->info.type == 'bat1') name = "Battery Low 1";
            else if (e->info.type == 'batF') name = "Battery Full";
            else if (e->info.type == 'chg0') name = "Battery Charging 0";
            else if (e->info.type == 'chg1') name = "Battery Charging 1";
            else if (e->info.type == 'glyC') name = "Glyph Charging";
            else if (e->info.type == 'glyP') name = "Glyph Plugin";
            else if (e->info.type == 'nsrv') name = "Need Service";

            if (name) {
                snprintf_(line_buf, sizeof(line_buf), "Dumped: %s SHSH", name);
            } else {
                char tag[5];
                tag[0] = (char)((e->info.type >> 24) & 0xFF);
                tag[1] = (char)((e->info.type >> 16) & 0xFF);
                tag[2] = (char)((e->info.type >> 8) & 0xFF);
                tag[3] = (char)(e->info.type & 0xFF);
                tag[4] = '\0';
                snprintf_(line_buf, sizeof(line_buf), "Dumped: %s SHSH", tag);
            }
            fb_print_line(line_buf, 0xFFFFFF);
        }
    }

    /* Kernelcache */
    if (dev.ios_version[0] < '5') {
        blobs[n++] = save_krnl();
        if (blobs[n - 1]) {
        fb_print_line("Dumped: Kernel SHSH", 0xFFFFFF);
        }
    }

    for (total = 0, i = 0; i < n; i++) {
        if (blobs[i]) {
            total += blobs[i]->size + offsetof(struct blob_t, data);
        }
    }

    data = malloc_(total);
    if (!data) {
        printf_("out of memory\n");
        fb_print_line("[!] Out of memory!", 0xFFFFFF);
        while (n-- > 0) {
            if (blobs[n]) {
                free_(blobs[n]);
            }
        }
        return 0;
    }

    for (total = 0, i = 0; i < n; i++) {
        if (blobs[i]) {
            unsigned size = blobs[i]->size + offsetof(struct blob_t, data);
            memcpy(data + total, blobs[i], size);
            total += size;
            free_(blobs[i]);
        }
    }

    get_addr = (unsigned)data;
    get_endp = get_addr + total;

    fb_print_line("=========================", 0xFFFFFF);
    fb_print_line("[+] SHSH Blobs Dumped!", 0xFFFFFF);

    LOG_DEBUG("Blobs copied at %x - %x (%u bytes)\n", get_addr, get_endp, total);
    timer_print_elapsed(t_start);
    return 0;
}


int
save_single_image(unsigned int type)
{
    unsigned int total;
    unsigned char *data;
    struct firmware_image *e;
    struct img3_root root;

    for (e = ((struct firmware_image *)image_list)->next; e != image_list; e = e->next) {
        if (e->info.type == type) {
            printf_("saving image %c%c%c%c\n",
                (type >> 24) & 0xFF, (type >> 16) & 0xFF, (type >> 8) & 0xFF, type & 0xFF);
            break;
        }
    }

    if (e == image_list) {
        printf_("image not found\n");
        return -1;
    }

    total = bdev_read(e->bdev, &root, e->offset_lo, sizeof(root));
    if (total != sizeof(root)) {
        printf_("image read err1\n");
        return -1;
    }

    data = malloc_(0x80 + root.size);
    if (!data) {
        printf_("out of memory\n");
        return 0;
    }

    total = bdev_read(e->bdev, data + 0x80, e->offset_lo, root.size);
    if (total != root.size) {
        free_(data);
        printf_("image read err2\n");
        return -1;
    }
    memcpy(data, data + 0x80 + root.shshOffset + 20 + 12, 0x80);
    decrypt_shsh(data);
    total += 0x80;

    get_addr = (unsigned)data;
    get_endp = get_addr + total;
    printf_("image copied at %x - %x\n", get_addr, get_endp);
    return 0;
}

#pragma GCC diagnostic pop
