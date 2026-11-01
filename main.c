/*
 *  Lynx32 - Clean & Verified 32-bit iBoot Dumper & Crypto Payload
 *  Features: dump (nand_llb, nand_fw, nor0, blob, mem, list), blob, aes, get
 */

#include "plib.h"
#include "nand.h"
#include "nand_fw.h"
#include "blob.h"
#include "dump.h"
#include "aes.h"
#include "fb.h"
#include "device.h"
#include "version.h"

#define TRANSFER_CHUNK 1000

unsigned int get_addr = 0;
unsigned int get_endp = 0x1000;
static char get_buf[TRANSFER_CHUNK * 2 + 16];

static int cmd_get(int argc, CmdArg *argv)
{
    if (argc > 1) {
        get_addr = argv[1].inthex;
        get_endp = get_addr + 0x1000;
        if (argc > 2) {
            get_endp = argv[2].inthex;
            if (get_endp < get_addr) {
                get_endp = get_addr + 0x1000;
            }
        }
        return 0;
    }

    if (get_addr < get_endp) {
        int i, chunk = get_endp - get_addr;
        if (chunk > TRANSFER_CHUNK) {
            chunk = TRANSFER_CHUNK;
        }
        i = snprintf_(get_buf, 16, "%u:%x:", chunk, get_addr);
        if (i) {
            hex2str(get_buf + i - 1, chunk, (void *)get_addr);
            if (create_envvar_("cmd-results", get_buf, 0) == 0) {
                get_addr += chunk;
                return 0;
            }
        }
    }

    create_envvar_("cmd-results", "end-of-transmission", 0);
    printf_("end-of-transmission\n");
    return 0;
}

static int cmd_help(void)
{
    printf_("Lynx32 commands:\n");
    printf_("        info    (i)      show device model, iOS version, ECID\n");
    printf_("        dump    (d)      dump <nand_llb | llb_raw | nand_fw | nor0 | blob | mem | list>\n");
    printf_("        blob    (b)      dump onboard SHSH/APTicket\n");
    printf_("        aes     (a)      aes <enc|dec> <uid|gid> <hexdata>\n");
    printf_("        get     (g)      get [addr [endp]] - stream buffer over USB\n");
    printf_("        fb                fb <info | clear [col] | rect <x y w h col> | text <x y msg> | demo>\n");
    printf_("        version (v)      check version lynx32\n");
    printf_("        help    (?)      show this help menu\n");
    
    return 0;
}

int _main(int argc, CmdArg *argv)
{
    if (!link(__builtin_return_address(0))) {
        return 0;
    }

    if (argc < 2) {
        cmd_help();
        return 0;
    }

    const char *cmd = argv[1].string;

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        return cmd_help();
    }

    if (strcmp(cmd, "info") == 0 || strcmp(cmd, "i") == 0 || strcmp(cmd, "dev") == 0) {
        return cmd_device_info(argc - 1, argv + 1);
    }

    if (strcmp(cmd, "dump") == 0 || strcmp(cmd, "d") == 0) {
        return cmd_dump(argc - 1, argv + 1);
    }

    if (strcmp(cmd, "blob") == 0 || strcmp(cmd, "b") == 0) {
#if DEBUG_LOG
        struct device_info dev;
        device_detect(&dev);
        if (dev.board[0]) {
            LOG_DEBUG("Device: %s (%s) | iOS: %s | ECID: %s\n", dev.model, dev.board, dev.ios_version, dev.ecid);
        } else {
            LOG_DEBUG("Device: %s | iOS: %s | ECID: %s\n", dev.model, dev.ios_version, dev.ecid);
        }
#endif
        return save_all_blobs();
    }

    if (strcmp(cmd, "fb") == 0) {
        return cmd_fb(argc - 1, argv + 1);
    }

    if (strcmp(cmd, "aes") == 0 || strcmp(cmd, "a") == 0) {
        return cmd_aes(argc - 1, argv + 1);
    }

    if (strcmp(cmd, "get") == 0 || strcmp(cmd, "g") == 0) {
        return cmd_get(argc - 1, argv + 1);
    }
    if (strcmp(cmd, "version") == 0 || strcmp(cmd, "v") == 0) {
        return cmd_version(argc - 1, argv + 1);
    }

    printf_("Unknown command: '%s'. Type 'help' for command list.\n", cmd);
    return 0;
}
