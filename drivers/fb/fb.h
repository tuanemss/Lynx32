#ifndef FB_H_included
#define FB_H_included

#include "plib.h"

struct fb_info {
    unsigned int base;     /* Framebuffer base address in RAM */
    unsigned int width;    /* Display width in pixels */
    unsigned int height;   /* Display height in pixels */
    unsigned int stride;   /* Bytes per row */
    unsigned int depth;    /* Color depth in bits (e.g. 32) */
    unsigned int size;     /* Total buffer size in bytes */
    int is_active;         /* 1 if detected, 0 otherwise */
    const char *method;    /* Detection strategy name */
};

/* Live dynamic probe */
int fb_probe(struct fb_info *info);

/* Framebuffer Primitives */
int fb_clear_screen(unsigned int hex_color);
int fb_draw_rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int hex_color);
int fb_draw_char(unsigned int x, unsigned int y, char c, unsigned int fg, unsigned int bg, unsigned int scale);
int fb_draw_string(unsigned int x, unsigned int y, const char *str, unsigned int fg, unsigned int bg, unsigned int scale);

/* On-Device LCD Console (iH8sn0w / iFaith style) */
void fb_init_console(const char *title, unsigned int title_color);
void fb_print_line(const char *str, unsigned int fg_color);

/* Command handler for 'go fb ...' */
int cmd_fb(int argc, CmdArg *argv);

#endif

