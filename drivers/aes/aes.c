/*
 *  Lynx32 - aes_crypto_cmd
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
#include "aes.h"

int cmd_aes(int argc, CmdArg *argv)
{
    if (argc < 4) {
        printf_("Usage: aes <enc|dec> <uid|gid> <hexdata>\n");
        return 0;
    }

    if (!aes_crypto_cmd_) {
        printf_("[!] aes_crypto_cmd symbol not resolved!\n");
        return -1;
    }

    int mode = (argv[1].string[0] == 'e') ? AES_ENCRYPT : AES_DECRYPT;
    int type = (argv[2].string[0] == 'u') ? UID_KEY : GID_KEY;
    unsigned char tmpbuf[KBAG_KEY_IV_SIZE];
    unsigned char outbuf[KBAG_KEY_IV_SIZE];
    char strbuf[KBAG_KEY_IV_SIZE * 2 + 1];

    int buflen = str2hex(sizeof(tmpbuf), tmpbuf, argv[3].string);
    if (buflen <= 0) {
        printf_("[!] Invalid hex data provided\n");
        return -1;
    }

    aes_crypto_cmd_(mode, (void *)tmpbuf, (void *)outbuf, buflen, type, NULL, NULL);
    hex2str(strbuf, buflen, outbuf);

    if (buflen == 48) {
        /* Standard Apple KBAG: 16 bytes IV (32 hex) + 32 bytes AES-256 Key (64 hex) */
        char keybuf[65];
        for (int i = 0; i < 64; i++) {
            keybuf[i] = strbuf[32 + i];
        }
        keybuf[64] = '\0';
        strbuf[32] = '\0';
        printf_("iv:  %s\n", strbuf);
        printf_("key: %s\n", keybuf);
    } else if (buflen == 32) {
        /* 16 bytes IV (32 hex) + 16 bytes AES-128 Key (32 hex) */
        char keybuf[33];
        for (int i = 0; i < 32; i++) {
            keybuf[i] = strbuf[32 + i];
        }
        keybuf[32] = '\0';
        strbuf[32] = '\0';
        printf_("iv:  %s\n", strbuf);
        printf_("key: %s\n", keybuf);
    } else {
        printf_("out: %s\n", strbuf);
    }
    return 0;
}
