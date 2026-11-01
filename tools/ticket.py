#!/usr/bin/env python3
"""
ticket.py - Standalone Onboard SHSH Converter for Lynx32 dumps
Adds full native support for 'nsrv' (NeedService) and all iOS 3/4/5+ components.
"""

import sys, os, struct, plistlib, zipfile

MAGIC_MAP = {
    0x696c6c62: "LLB",              # 'illb'
    0x6b726e6c: "KernelCache",      # 'krnl'
    0x6c6f676f: "AppleLogo",        # 'logo'
    0x64747265: "DeviceTree",       # 'dtre'
    0x69626f74: "iBoot",            # 'ibot'
    0x7265636d: "RecoveryMode",     # 'recm'
    0x6e737276: "NeedService",      # 'nsrv'
    0x62617430: "BatteryLow0",      # 'bat0'
    0x62617431: "BatteryLow1",      # 'bat1'
    0x62617446: "BatteryFull",      # 'batF'
    0x63686730: "BatteryCharging0", # 'chg0'
    0x63686731: "BatteryCharging1", # 'chg1'
    0x676c7943: "BatteryCharging",  # 'glyC'
    0x676c7950: "BatteryPlugin",    # 'glyP'
}

def find_partial_digest(manifest_dict, comp_name):
    for identity in manifest_dict.get("BuildIdentities", []):
        manifest = identity.get("Manifest", {})
        if comp_name in manifest and "PartialDigest" in manifest[comp_name]:
            return manifest[comp_name]["PartialDigest"]
    return None

def make_blob(ecid_bytes, cert_bytes, sig_bytes):
    shsh_hdr = struct.pack("<III", 0x48534853, len(sig_bytes) + 12, len(sig_bytes))
    return ecid_bytes + shsh_hdr + sig_bytes + cert_bytes

def convert_dump(raw_path, out_shsh_path, ipsw_or_plist_path):
    if ipsw_or_plist_path.endswith(".ipsw") or zipfile.is_zipfile(ipsw_or_plist_path):
        with zipfile.ZipFile(ipsw_or_plist_path, "r") as zf:
            manifest_data = zf.read("BuildManifest.plist")
    else:
        with open(ipsw_or_plist_path, "rb") as f:
            manifest_data = f.read()

    manifest = plistlib.loads(manifest_data)

    with open(raw_path, "rb") as f:
        dump_data = f.read()

    pos = 0
    llb_full_blob = None
    ecid_tag = None
    cert_tag = None

    while pos + 8 <= len(dump_data):
        magic, size = struct.unpack("<II", dump_data[pos:pos+8])
        chunk = dump_data[pos+8 : pos+8+size]

        if magic == 0x696c6c62 and size > 200:
            llb_full_blob = chunk
            p = 0
            while p + 12 <= len(chunk):
                tag, tlen, dlen = struct.unpack("<III", chunk[p:p+12])
                if chunk[p:p+4] in [b'DICE', b'ECID']:
                    ecid_tag = chunk[p : p+tlen]
                elif chunk[p:p+4] in [b'TREC', b'CERT']:
                    cert_tag = chunk[p : p+tlen]
                p += tlen
            break
        pos += 8 + size

    if not llb_full_blob or not ecid_tag or not cert_tag:
        print("[-] Error: Full LLB or ECID/CERT tags not found in dump!")
        sys.exit(1)

    ecid_num = struct.unpack("<Q", ecid_tag[12:20])[0]
    print(f"[+] Found device ECID: 0x{ecid_num:016X} ({ecid_num})")

    shsh_dict = {}
    pos = 0

    while pos + 8 <= len(dump_data):
        magic, size = struct.unpack("<II", dump_data[pos:pos+8])
        chunk = dump_data[pos+8 : pos+8+size]

        if magic in [0x53434142, 0x42414353]: # 'SCAB'
            shsh_dict["APTicket"] = chunk
            print("[+] Processed APTicket (SCAB)")
        elif magic in MAGIC_MAP:
            comp_name = MAGIC_MAP[magic]
            pd = find_partial_digest(manifest, comp_name)
            if pd:
                if magic == 0x696c6c62:
                    if size > 200:
                        blob_data = llb_full_blob
                    else:
                        pos += 8 + size
                        continue
                else:
                    blob_data = make_blob(ecid_tag, cert_tag, chunk)

                shsh_dict[comp_name] = {
                    "Blob": blob_data,
                    "PartialDigest": pd
                }
                print(f"[+] Processed {comp_name:<18} (magic: 0x{magic:08x})")
        else:
            print(f"[!] Warning: Unknown magic 0x{magic:08x}, skipping")

        pos += 8 + size

    with open(out_shsh_path, "wb") as f:
        plistlib.dump(shsh_dict, f)

    print(f"[+] Successfully converted {len(shsh_dict)} components into: {out_shsh_path}")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} <blob.raw> <output.shsh> <BuildManifest.plist or IPSW>")
        sys.exit(1)
    convert_dump(sys.argv[1], sys.argv[2], sys.argv[3])
