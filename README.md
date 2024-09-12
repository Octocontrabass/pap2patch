# Linksys PAP2 Firmware Patch

To remove the password prompt from the web server, change the instruction at
memory address `0xed2c0` from `0x60053019` to `0x60013019`. It's at offset
`0x12c0` in the decompressed data. Recompressing is just gzip without the
header. Update the hashes and checksums to flash the modified firmware.

If there's already a password in place, poke the RAM with something metal to
crash it into recovery mode (where it flashes SMS in Morse code) and use the
recovery tool to flash it. The firmware binary inside the recovery tool can be
replaced easily with a hex editor. If the new firmware is bigger, you might
need to use the old Sipura SPA-2000 crossflash to temporarily bypass the admin
password. I did it this way, since I figured out how to crash into recovery
mode before I figured out how to patch the firmware.

Goodbye carrier locks. Hello [modem over IP](https://gekk.info/articles/ata-config.html).
