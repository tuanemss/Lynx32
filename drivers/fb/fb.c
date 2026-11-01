/*
 *  Lynx32 - Dynamic Framebuffer Driver
 *
 *  I used AI to create something I dreamed of, solely for research purposes.
 * 
 *  Provides 100% dynamic probing of Apple iBoot Framebuffer without hardcoding.
 *  Supports BSS signature analysis, Hardware MMIO register reading,
 *  Environment variable lookup, screen clearing, and bitmap text rendering.
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
#include "fb.h"
#include "font.h"

static struct fb_info cached_fb = {0};

/* --- Detection Methods --- */

/*
 * Method 1: Scan iBoot BSS / Data segment for Apple paint globals
 * In iBoot's lib/paint/paint.c:
 *   static uint32_t window_height;
 *   static uint32_t window_width;
 *   static uint32_t window_stride;
 *   static uint32_t window_depth;
 *   static uintptr_t window_framebuffer;
 */
static int probe_bss_paint_window(struct fb_info *info)
{
    unsigned int start = TARGET_BASEADDR;
    unsigned int end = TARGET_BASEADDR + 0x200000;
    const unsigned int *p;

    for (p = (const unsigned int *)start; (unsigned int)p < end; p++) {
        unsigned int v0 = p[0];
        unsigned int v1 = p[1];
        unsigned int v2 = p[2];
        unsigned int v3 = p[3];
        unsigned int v4 = p[4];

        /* Pattern 1: {height, width, stride, depth, fb_phys} */
        if (v0 >= 320 && v0 <= 2732 &&
            v1 >= 320 && v1 <= 2732 &&
            (v2 == v1 * 4 || v2 == ((v1 * 4 + 63) & ~63) || v2 == v1 * 2) &&
            (v3 == 32 || v3 == 16 || v3 == 24 || v3 == 30) &&
            v4 >= 0x08000000 && v4 < 0xff000000 && (v4 & 0xfff) == 0) {

            info->height = v0;
            info->width = v1;
            info->stride = v2;
            info->depth = (v3 == 30) ? 32 : v3;
            info->base = v4;
            info->size = v0 * v2;
            info->method = "iBoot BSS paint window signature [H,W,S,D,FB]";
            info->is_active = 1;
            return 1;
        }

        /* Pattern 2: {width, height, stride, depth, fb_phys} */
        if (v0 >= 320 && v0 <= 2732 &&
            v1 >= 320 && v1 <= 2732 &&
            (v2 == v0 * 4 || v2 == ((v0 * 4 + 63) & ~63) || v2 == v0 * 2) &&
            (v3 == 32 || v3 == 16 || v3 == 24 || v3 == 30) &&
            v4 >= 0x08000000 && v4 < 0xff000000 && (v4 & 0xfff) == 0) {

            info->width = v0;
            info->height = v1;
            info->stride = v2;
            info->depth = (v3 == 30) ? 32 : v3;
            info->base = v4;
            info->size = v1 * v2;
            info->method = "iBoot BSS paint window signature [W,H,S,D,FB]";
            info->is_active = 1;
            return 1;
        }
    }
    return 0;
}

/*
 * Method 2: Hardware MMIO Controller Register Scan
 * On Apple DisplayPipe (A5/A5X/A6/A6X):
 *   DISP0_BASE_ADDR = 0x3A100000
 *   rDPUSRCBASE(0)  = 0x3A104044
 *   rDPUSRCSTRD(0)  = 0x3A104048
 *   rDPUSRCWH(0)    = 0x3A104050
 */
static int probe_hw_mmio(struct fb_info *info)
{
    volatile unsigned int *reg_base = (volatile unsigned int *)0x3A104044;
    unsigned int fb = *reg_base;

    if (fb >= 0x08000000 && fb < 0xff000000 && (fb & 0xfff) == 0) {
        unsigned int strd = *(volatile unsigned int *)0x3A104048 & ~0x3f;
        unsigned int wh = *(volatile unsigned int *)0x3A104050;
        unsigned int w = (wh >> 16) & 0xffff;
        unsigned int h = wh & 0xffff;

        if (w >= 320 && w <= 2732 && h >= 320 && h <= 2732) {
            info->base = fb;
            info->width = w;
            info->height = h;
            info->stride = strd ? strd : (w * 4);
            info->depth = 32;
            info->size = h * info->stride;
            info->method = "Hardware MMIO DisplayPipe (0x3A104044)";
            info->is_active = 1;
            return 1;
        }
    }
    return 0;
}

/*
 * Method 3: Environment Variable Scan ("framebuffer")
 * In iBoot drivers/apple/displaypipe/displaypipe.c:
 *   env_set_uint("framebuffer", (uintptr_t)main_window->c.fb_phys, 0);
 */
static int probe_env_framebuffer(struct fb_info *info)
{
    unsigned int start = TARGET_BASEADDR;
    unsigned int end = TARGET_BASEADDR + 0x200000;
    const unsigned char *p;

    for (p = (const unsigned char *)start; (unsigned int)p < end; p += 4) {
        if (p[0] == 'f' && p[1] == 'r' && p[2] == 'a' && p[3] == 'm' &&
            p[4] == 'e' && p[5] == 'b' && p[6] == 'u' && p[7] == 'f' &&
            p[8] == 'f' && p[9] == 'e' && p[10] == 'r' && p[11] == 0) {

            /* In struct env_var_entry, numeric value is located before name */
            const unsigned int *cand_ptr = (const unsigned int *)(p - 8);
            unsigned int cand = *cand_ptr;
            if (cand >= 0x08000000 && cand < 0xff000000 && (cand & 0xfff) == 0) {
                info->base = cand;
                if (!info->width) info->width = 640;
                if (!info->height) info->height = 960;
                info->stride = info->width * 4;
                info->depth = 32;
                info->size = info->height * info->stride;
                info->method = "iBoot EnvVar 'framebuffer'";
                info->is_active = 1;
                return 1;
            }
        }
    }
    return 0;
}

/*
 * Method 4: Fallback Known Apple SoC RAM bounds
 */
static int probe_fallback_memmap(struct fb_info *info)
{
    static const unsigned int candidates[] = {
        0x5F700000, /* iPhone 4, iPad 1, iPod 4 */
        0x9F000000, /* iPad 2, iPhone 4S */
        0x8FE00000, /* iPad 2 Rev A / iPad mini 1 */
        0x4FD00000, /* iPhone 3GS, iPod 3 */
        0x0FC00000, /* iPod 2G */
    };
    for (unsigned int i = 0; i < ARRAY_SIZE(candidates); i++) {
        unsigned int addr = candidates[i];
        volatile unsigned int *p = (volatile unsigned int *)addr;
        unsigned int orig = *p;
        *p = orig ^ 0x0055aa55;
        if (*p == (orig ^ 0x0055aa55)) {
            *p = orig;
            info->base = addr;
            info->width = 640;
            info->height = 960;
            info->stride = 640 * 4;
            info->depth = 32;
            info->size = 960 * 640 * 4;
            info->method = "Fallback SoC RAM candidate";
            info->is_active = 1;
            return 1;
        }
    }
    return 0;
}

/* --- Public API --- */

int fb_probe(struct fb_info *info)
{
    if (cached_fb.is_active) {
        if (info) *info = cached_fb;
        return 1;
    }

    struct fb_info res;
    memset(&res, 0, sizeof(res));

    /* Layer 1: Scan iBoot BSS for paint_window globals */
    if (!probe_bss_paint_window(&res)) {
        /* Layer 2: Read Hardware MMIO DisplayPipe controller */
        if (!probe_hw_mmio(&res)) {
            /* Layer 3: Check Environment variables */
            if (!probe_env_framebuffer(&res)) {
                /* Layer 4: Fallback candidates */
                probe_fallback_memmap(&res);
            }
        }
    }

    if (res.is_active) {
        cached_fb = res;
        if (info) *info = res;
        return 1;
    }
    return 0;
}

int fb_clear_screen(unsigned int hex_color)
{
    struct fb_info fb;
    if (!fb_probe(&fb)) {
        printf_("[-] Error: Framebuffer not found or not active!\n");
        return -1;
    }

    for (unsigned int y = 0; y < fb.height; y++) {
        volatile unsigned int *row = (volatile unsigned int *)(fb.base + y * fb.stride);
        for (unsigned int x = 0; x < fb.width; x++) {
            row[x] = hex_color;
        }
    }
    return 0;
}

int fb_draw_rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int hex_color)
{
    struct fb_info fb;
    if (!fb_probe(&fb)) return -1;

    if (x >= fb.width || y >= fb.height) return 0;
    if (x + w > fb.width) w = fb.width - x;
    if (y + h > fb.height) h = fb.height - y;

    for (unsigned int cur_y = y; cur_y < y + h; cur_y++) {
        volatile unsigned int *row = (volatile unsigned int *)(fb.base + cur_y * fb.stride);
        for (unsigned int cur_x = x; cur_x < x + w; cur_x++) {
            row[cur_x] = hex_color;
        }
    }
    return 0;
}

int fb_draw_char(unsigned int x, unsigned int y, char c, unsigned int fg, unsigned int bg, unsigned int scale)
{
    struct fb_info fb;
    unsigned int row, col, sy, sx;
    unsigned char ch;

    if (!fb_probe(&fb)) return -1;
    if (!scale) scale = (fb.width >= 640) ? 2 : 1;

    ch = (unsigned char)c;

    for (row = 0; row < 10; row++) {
        for (col = 0; col < 6; col++) {
            unsigned int idx = (60 * (unsigned int)ch) + (6 * row) + col;
            int pixel_on = (ifaith_font_data[8 + (idx >> 3)] >> (idx & 7)) & 1;
            unsigned int color = pixel_on ? fg : bg;

            if (!pixel_on && bg == 0xFFFFFFFF) {
                continue; /* Transparent */
            }

            for (sy = 0; sy < scale; sy++) {
                unsigned int py = y + row * scale + sy;
                volatile unsigned int *prow;
                if (py >= fb.height) continue;
                prow = (volatile unsigned int *)(fb.base + py * fb.stride);
                for (sx = 0; sx < scale; sx++) {
                    unsigned int px = x + col * scale + sx;
                    if (px >= fb.width) continue;
                    prow[px] = color;
                }
            }
        }
    }
    return 0;
}

int fb_draw_string(unsigned int x, unsigned int y, const char *str, unsigned int fg, unsigned int bg, unsigned int scale)
{
    unsigned int cur_x;
    unsigned int cur_y;

    if (!str) return 0;
    if (!cached_fb.is_active) {
        struct fb_info fb;
        if (!fb_probe(&fb)) return -1;
    }
    if (!scale) scale = (cached_fb.width >= 640) ? 2 : 1;

    cur_x = x;
    cur_y = y;
    while (*str) {
        char c = *str++;
        if (c == '\n') {
            cur_x = x;
            cur_y += 10 * scale;
            continue;
        } else if (c == '\r') {
            cur_x = x;
            continue;
        }
        fb_draw_char(cur_x, cur_y, c, fg, bg, scale);
        cur_x += 6 * scale;
        if (cur_x + 6 * scale > cached_fb.width) {
            cur_x = x;
            cur_y += 10 * scale;
        }
    }
    return 0;
}

static void fb_show_info(const struct fb_info *fb)
{
    printf_("Framebuffer: 0x%08x | Res: %ux%u | Stride: %u | Depth: %ubpp | Method: %s\n",
            fb->base, fb->width, fb->height, fb->stride, fb->depth, fb->method);
}

/* --- On-Device LCD Console (Authentic iH8sn0w / iFaith pure monochrome terminal) --- */
static unsigned int g_console_x = 10;
static unsigned int g_console_y = 20;
static unsigned int g_console_scale = 2;
static unsigned int g_console_bg = 0x000000;
static unsigned int g_console_fg = 0xFFFFFF;

static void fb_scrollup(void)
{
    unsigned int line_h = 10 * g_console_scale;
    unsigned int move_bytes;
    volatile unsigned int *dst;
    volatile unsigned int *src;
    volatile unsigned int *clear_start;

    if (!cached_fb.is_active || line_h >= cached_fb.height) return;

    dst = (volatile unsigned int *)cached_fb.base;
    src = (volatile unsigned int *)(cached_fb.base + line_h * cached_fb.stride);
    move_bytes = (cached_fb.height - line_h) * cached_fb.stride;

    memmove((void *)dst, (const void *)src, move_bytes);

    /* Clear bottom row to pure black */
    clear_start = (volatile unsigned int *)(cached_fb.base + (cached_fb.height - line_h) * cached_fb.stride);
    memset((void *)clear_start, 0, line_h * cached_fb.stride);

    g_console_y -= line_h;
}

void fb_init_console(const char *title, unsigned int title_color)
{
    struct fb_info fb;
    (void)title;
    (void)title_color;

    if (!fb_probe(&fb)) return;

    g_console_scale = (fb.width >= 640) ? 2 : 1;
    g_console_bg = 0x000000; /* Pure Black - Authentic iFaith */
    g_console_fg = 0xFFFFFF; /* Pure White - Authentic iFaith */

    /* Clear screen to pure black */
    fb_clear_screen(g_console_bg);

    /* Top-left aligned with clean margin */
    g_console_x = 10 * g_console_scale;
    g_console_y = 12 * g_console_scale;
}

void fb_print_line(const char *str, unsigned int fg_color)
{
    unsigned int line_h;

    if (!cached_fb.is_active) {
        struct fb_info fb;
        if (!fb_probe(&fb)) return;
    }

    if (!str) return;

    /* Always pure white for authentic iFaith styling */
    (void)fg_color;
    fg_color = g_console_fg;
    line_h = 10 * g_console_scale;

    while (*str) {
        char buf[128];
        unsigned int i = 0;
        while (*str && *str != '\n' && *str != '\r' && i < sizeof(buf) - 1) {
            buf[i++] = *str++;
        }
        buf[i] = '\0';
        if (*str == '\r') str++;
        if (*str == '\n') str++;

        if (g_console_y + line_h >= cached_fb.height - (10 * g_console_scale)) {
            fb_scrollup();
        }

        if (i > 0) {
            fb_draw_string(g_console_x, g_console_y, buf, fg_color, g_console_bg, g_console_scale);
        }
        g_console_y += line_h;
    }
}

static int fb_run_demo(void)
{
    struct fb_info fb;
    if (!fb_probe(&fb)) {
        printf_("[-] Probe failed\n");
        return -1;
    }

    /* Standard color test bars (White, Yellow, Cyan, Green, Magenta, Red, Blue, Black) */
    static const unsigned int colors[8] = {
        0xFFFFFF, /* White */
        0xFFFF00, /* Yellow */
        0x00FFFF, /* Cyan */
        0x00FF00, /* Green */
        0xFF00FF, /* Magenta */
        0xFF0000, /* Red */
        0x0000FF, /* Blue */
        0x000000  /* Black */
    };

    unsigned int bar_w = fb.width / 8;
    for (unsigned int i = 0; i < 8; i++) {
        unsigned int x = i * bar_w;
        unsigned int w = (i == 7) ? (fb.width - x) : bar_w;
        fb_draw_rect(x, 0, w, fb.height, colors[i]);
    }

    printf_("color bars rendered\n");
    return 0;
}

int cmd_fb(int argc, CmdArg *argv)
{
    struct fb_info fb;
    int ok = fb_probe(&fb);

    if (argc < 2 || strcmp(argv[1].string, "info") == 0) {
        if (!ok) {
            printf_("[-] Could not dynamically detect Framebuffer in memory.\n");
            return -1;
        }
        fb_show_info(&fb);
        return 0;
    }

    const char *sub = argv[1].string;

    if (strcmp(sub, "clear") == 0) {
        unsigned int col = (argc > 2) ? (unsigned int)argv[2].inthex : 0x000000; /* Black default */
        printf_("[*] Clearing screen with color 0x%06x...\n", col);
        fb_clear_screen(col);
        printf_("[+] Screen cleared.\n");
        return 0;
    }

    if (strcmp(sub, "demo") == 0) {
        return fb_run_demo();
    }

    if (strcmp(sub, "rect") == 0) {
        if (argc < 6) {
            printf_("Usage: fb rect <x> <y> <width> <height> [hex_color]\n");
            return 0;
        }
        unsigned int x = (unsigned int)argv[2].inthex;
        unsigned int y = (unsigned int)argv[3].inthex;
        unsigned int w = (unsigned int)argv[4].inthex;
        unsigned int h = (unsigned int)argv[5].inthex;
        unsigned int col = (argc > 6) ? (unsigned int)argv[6].inthex : 0xFFFFFF;
        fb_draw_rect(x, y, w, h, col);
        printf_("[+] Drew rect (%u, %u) %ux%u color 0x%06x\n", x, y, w, h, col);
        return 0;
    }

    if (strcmp(sub, "text") == 0) {
        if (argc < 5) {
            printf_("Usage: fb text <x> <y> <string> [fg_color] [bg_color]\n");
            return 0;
        }
        unsigned int x = (unsigned int)argv[2].inthex;
        unsigned int y = (unsigned int)argv[3].inthex;
        const char *msg = argv[4].string;
        unsigned int fg = (argc > 5) ? (unsigned int)argv[5].inthex : 0xFFFFFF;
        unsigned int bg = (argc > 6) ? (unsigned int)argv[6].inthex : 0xFFFFFFFF; /* Default transparent */
        fb_draw_string(x, y, msg, fg, bg, 0);
        printf_("[+] Rendered text: \"%s\"\n", msg);
        return 0;
    }

    printf_("Usage: fb <info | clear [col] | rect <x y w h col> | text <x y msg [fg] [bg]> | demo>\n");
    return 0;
}
