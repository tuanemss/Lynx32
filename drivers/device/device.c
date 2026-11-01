/*
 *  Lynx32 - Device Identity & System Version Library
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
#include "device.h"

struct device_map {
    char id[16];
    char name[32];
};

static const struct device_map g_model_map[] = {
    { "iPhone1,1", "iPhone 2G" },
    { "iPhone1,2", "iPhone 3G" },
    { "iPhone2,1", "iPhone 3GS" },
    { "iPhone3,1", "iPhone 4 (GSM)" },
    { "iPhone3,2", "iPhone 4 (Rev A)" },
    { "iPhone3,3", "iPhone 4 [CDMA]" },
    { "iPhone4,1", "iPhone 4S" },
    { "iPhone5,1", "iPhone 5 (GSM)" },
    { "iPhone5,2", "iPhone 5 (Global)" },
    { "iPhone5,3", "iPhone 5c (GSM)" },
    { "iPhone5,4", "iPhone 5c (Global)" },
    { "iPod1,1",   "iPod Touch 1G" },
    { "iPod2,1",   "iPod Touch 2G" },
    { "iPod3,1",   "iPod Touch 3G" },
    { "iPod4,1",   "iPod Touch 4" },
    { "iPod5,1",   "iPod Touch 5" },
    { "iPad1,1",   "iPad 1" },
    { "iPad2,1",   "iPad 2 (WiFi)" },
    { "iPad2,2",   "iPad 2 (GSM)" },
    { "iPad2,3",   "iPad 2 (CDMA)" },
    { "iPad2,4",   "iPad 2 (Mid 2012)" },
    { "iPad2,5",   "iPad mini (WiFi)" },
    { "iPad2,6",   "iPad mini (GSM)" },
    { "iPad2,7",   "iPad mini (Global)" },
    { "iPad3,1",   "iPad 3 (WiFi)" },
    { "iPad3,2",   "iPad 3 (CDMA)" },
    { "iPad3,3",   "iPad 3 (GSM)" },
    { "iPad3,4",   "iPad 4 (WiFi)" },
    { "iPad3,5",   "iPad 4 (GSM)" },
    { "iPad3,6",   "iPad 4 (Global)" },
    { "AppleTV2,1","Apple TV 2" },
    { "AppleTV3,1","Apple TV 3" },
    { "AppleTV3,2","Apple TV 3 (Rev A)" },
    { "", "" }
};

static const struct device_map g_board_map[] = {
    { "n90b",  "iPhone 4 (Rev A)" },
    { "n90",   "iPhone 4 (GSM)" },
    { "n92",   "iPhone 4 [CDMA]" },
    { "n94",   "iPhone 4S" },
    { "n41",   "iPhone 5 (GSM)" },
    { "n42",   "iPhone 5 (Global)" },
    { "n48",   "iPhone 5c (GSM)" },
    { "n49",   "iPhone 5c (Global)" },
    { "n88",   "iPhone 3GS" },
    { "n82",   "iPhone 3G" },
    { "m68",   "iPhone 2G" },
    { "n78",   "iPod Touch 5" },
    { "n81",   "iPod Touch 4" },
    { "n18",   "iPod Touch 3G" },
    { "n72",   "iPod Touch 2G" },
    { "n45",   "iPod Touch 1G" },
    { "k48",   "iPad 1" },
    { "k93a",  "iPad 2 (Mid 2012)" },
    { "k93",   "iPad 2 (WiFi)" },
    { "k94",   "iPad 2 (GSM)" },
    { "k95",   "iPad 2 (CDMA)" },
    { "p105",  "iPad mini (WiFi)" },
    { "p106",  "iPad mini (GSM)" },
    { "p107",  "iPad mini (Global)" },
    { "j1",    "iPad 3 (WiFi)" },
    { "j2a",   "iPad 3 (GSM)" },
    { "j2",    "iPad 3 (CDMA)" },
    { "p101",  "iPad 4 (WiFi)" },
    { "p102",  "iPad 4 (GSM)" },
    { "p103",  "iPad 4 (Global)" },
    { "k66",   "Apple TV 2" },
    { "j33i",  "Apple TV 3 (Rev A)" },
    { "j33",   "Apple TV 3" },
    { "", "" }
};

static char g_ecid[32] = "0000000000000000";
static char g_model[32] = "";
static char g_ios[32] = "";
static char g_board[16] = "";
static int g_boot_mounted = 0;

void reset_boot_mount(void)
{
    /* Keep mount state persistent across commands to avoid "mountpoint already occupied" */
}

void ensure_boot_mounted(void)
{
    if (!g_boot_mounted) {
        fs_mount_("nand0a", "hfs", "/boot");
        g_boot_mounted = 1;
    }
}

void device_feed_llb_data(const unsigned char *data, unsigned int len)
{
    unsigned int i;
    for (i = 0; i + 20 <= len; i++) {
        if ((data[i] == 'E' && data[i+1] == 'C' && data[i+2] == 'I' && data[i+3] == 'D') ||
            (data[i] == 'D' && data[i+1] == 'I' && data[i+2] == 'C' && data[i+3] == 'E')) {
            unsigned int ecid_lo = (unsigned int)data[i+12] | ((unsigned int)data[i+13] << 8) |
                                   ((unsigned int)data[i+14] << 16) | ((unsigned int)data[i+15] << 24);
            unsigned int ecid_hi = (unsigned int)data[i+16] | ((unsigned int)data[i+17] << 8) |
                                   ((unsigned int)data[i+18] << 16) | ((unsigned int)data[i+19] << 24);
            if (ecid_lo || ecid_hi) {
                snprintf_(g_ecid, sizeof(g_ecid), "%08X%08X", ecid_hi, ecid_lo);
                return;
            }
        }
    }
}

static void detect_ecid_from_memory(void)
{
    const char *base = (const char *)TARGET_BASEADDR;
    unsigned int i;
    if (strcmp(g_ecid, "0000000000000000") != 0) {
        return;
    }
    for (i = 0; i < 0x100000; i++) {
        if (base[i] == 'E' && base[i+1] == 'C' && base[i+2] == 'I' && base[i+3] == 'D' && base[i+4] == ':') {
            const char *p = &base[i + 5];
            int valid = 1;
            int j;
            for (j = 0; j < 16; j++) {
                char c = p[j];
                if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
                    valid = 0;
                    break;
                }
            }
            if (valid) {
                for (j = 0; j < 16; j++) {
                    char c = p[j];
                    if (c >= 'a' && c <= 'f') c = c - 'a' + 'A';
                    g_ecid[j] = c;
                }
                g_ecid[16] = '\0';
                return;
            }
        }
    }
}

static int get_plist_key_string(const char *xml, unsigned int xml_len, const char *key, char *out, unsigned int out_len)
{
    char key_tag[64];
    const char *p;
    const char *start;
    const char *end;
    unsigned int val_len;

    snprintf_(key_tag, sizeof(key_tag), "<key>%s</key>", key);
    p = (const char *)memmem(xml, xml_len, key_tag, strlen(key_tag));
    if (!p) return -1;

    p += strlen(key_tag);
    start = (const char *)memmem(p, xml_len - (p - xml), "<string>", 8);
    if (!start) return -1;
    start += 8;

    end = (const char *)memmem(start, xml_len - (start - xml), "</string>", 9);
    if (!end) return -1;

    val_len = (unsigned int)(end - start);
    if (val_len >= out_len) val_len = out_len - 1;
    memcpy(out, start, val_len);
    out[val_len] = '\0';
    return 0;
}

static int detect_ios_from_rootfs(char *out, unsigned int max_len)
{
    char *plist = (char *)TARGET_LOADADDR + 0x500000;
    unsigned int size = 0x10000;
    char prod_ver[32];
    char build_ver[32];

    ensure_boot_mounted();
    if (fs_loadfile_("/boot/System/Library/CoreServices/SystemVersion.plist", plist, &size) != 0 || size == 0) {
        return -1;
    }

    if (size >= 0x10000) size = 0x10000 - 1;
    plist[size] = '\0';

    prod_ver[0] = '\0';
    build_ver[0] = '\0';

    get_plist_key_string(plist, size, "ProductVersion", prod_ver, sizeof(prod_ver));
    get_plist_key_string(plist, size, "ProductBuildVersion", build_ver, sizeof(build_ver));

    if (prod_ver[0] && build_ver[0]) {
        snprintf_(out, max_len, "%s (%s)", prod_ver, build_ver);
        return 0;
    } else if (prod_ver[0]) {
        snprintf_(out, max_len, "%s", prod_ver);
        return 0;
    }
    return -1;
}

static void detect_device_name(char *out, unsigned int max_len)
{
    const char *base = (const char *)TARGET_BASEADDR;
    const struct device_map *map;
    const char *p;
    unsigned int i;

    /* Extract board directly from iBoot banner: "iBoot-xxx for <board>ap," */
    p = (const char *)memmem(base + 0x100, 0x1f00, " for ", 5);
    if (p) {
        const char *b_start = p + 5;
        char extracted[16];
        unsigned int b_len = 0;

        while (b_len < sizeof(extracted) - 1 &&
               b_start[b_len] != ',' &&
               b_start[b_len] != ' ' &&
               b_start[b_len] != '\0' &&
               b_start[b_len] != '\r' &&
               b_start[b_len] != '\n') {
            extracted[b_len] = b_start[b_len];
            b_len++;
        }
        extracted[b_len] = '\0';

        if (b_len > 0) {
            snprintf_(g_board, sizeof(g_board), "%s", extracted);

            for (map = g_board_map; map->id[0] != '\0'; map++) {
                unsigned int id_len = strlen(map->id);
                if (memcmp(extracted, map->id, id_len) == 0) {
                    snprintf_(out, max_len, "%s", map->name);
                    return;
                }
            }

            snprintf_(out, max_len, "Apple Device (%s)", extracted);
            return;
        }
    }

    /* Scan iBoot banner for board names */
    for (i = 0x100; i < 0x2000; i++) {
        for (map = g_board_map; map->id[0] != '\0'; map++) {
            unsigned int id_len = strlen(map->id);
            if (memcmp(&base[i], map->id, id_len) == 0) {
                char next = base[i + id_len];
                if (next == ',' || next == ' ' || (next == 'a' && base[i + id_len + 1] == 'p')) {
                    snprintf_(out, max_len, "%s", map->name);
                    snprintf_(g_board, sizeof(g_board), "%sap", map->id);
                    return;
                }
            }
        }
    }

    /* Check for product model identifiers anywhere in first 512KB */
    for (map = g_model_map; map->id[0] != '\0'; map++) {
        unsigned int id_len = strlen(map->id);
        if (id_len > 0 && memmem(base, 0x80000, map->id, id_len)) {
            snprintf_(out, max_len, "%s", map->name);
            return;
        }
    }

    snprintf_(out, max_len, "NULL");
}


static void detect_ios_version(char *out, unsigned int max_len)
{

    if (detect_ios_from_rootfs(out, max_len) == 0) {
        return;
    }

    snprintf_(out, max_len, "NULL");
}

int device_detect(struct device_info *info)
{
    detect_ecid_from_memory();
    detect_device_name(g_model, sizeof(g_model));
    detect_ios_version(g_ios, sizeof(g_ios));

    if (info) {
        snprintf_(info->model, sizeof(info->model), "%s", g_model);
        snprintf_(info->ios_version, sizeof(info->ios_version), "%s", g_ios);
        snprintf_(info->ecid, sizeof(info->ecid), "%s", g_ecid);
        snprintf_(info->board, sizeof(info->board), "%s", g_board);
    }
    return 0;
}

const char *device_get_model(void)
{
    if (g_model[0] == '\0') {
        device_detect(NULL);
    }
    return g_model;
}

const char *device_get_ios(void)
{
    if (g_ios[0] == '\0') {
        device_detect(NULL);
    }
    return g_ios;
}

const char *device_get_ecid(void)
{
    if (strcmp(g_ecid, "0000000000000000") == 0) {
        detect_ecid_from_memory();
    }
    return g_ecid;
}

int cmd_device_info(int argc, CmdArg *argv)
{
    (void)argc;
    (void)argv;
    struct device_info dev;
    device_detect(&dev);

    if (dev.board[0]) {
        printf_("Device: %s (%s) | iOS: %s | ECID: %s\n", dev.model, dev.board, dev.ios_version, dev.ecid);
    } else {
        printf_("Device: %s | iOS: %s | ECID: %s\n", dev.model, dev.ios_version, dev.ecid);
    }
    return 0;
}
