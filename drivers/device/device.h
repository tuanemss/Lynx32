#ifndef DEVICE_H_included
#define DEVICE_H_included

#include "plib.h"

struct device_info {
    char model[32];        /* e.g. "iPod Touch 5", "iPhone 4 (GSM)" */
    char ios_version[32];  /* e.g. "6.0.1 (10A523)" */
    char ecid[32];         /* e.g. "000001FAE7062DCE" */
    char board[16];        /* e.g. "n78ap", "n90ap" */
};

/* Mount rootfs cleanly if not already mounted */
void ensure_boot_mounted(void);
void reset_boot_mount(void);

/* Feed LLB data to extract ECID if available */
void device_feed_llb_data(const unsigned char *data, unsigned int len);

/* Detect all device information (Model, iOS, ECID, Board) */
int device_detect(struct device_info *info);

/* Direct getters */
const char *device_get_model(void);
const char *device_get_ios(void);
const char *device_get_ecid(void);

/* Command handler for 'go info' / 'go dev' */
int cmd_device_info(int argc, CmdArg *argv);

#endif
