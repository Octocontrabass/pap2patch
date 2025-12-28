# Linksys PAP2 Firmware Patch

This patch applies to PAP2 firmware 3.1.23, which is the newest version.

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

* [PAP2 firmware 3.1.23 from Cisco.](https://community.cisco.com/t5/voice-systems-and-accessories-small-business/same-firmware-for-pap2-and-pap2t/m-p/1240279/highlight/true#M65182)
* [PAP2 recovery tool from Cisco.](https://community.cisco.com/t5/voice-systems-and-accessories-small-business/pap2-na-in-sos-mode-help-needed/m-p/1399412/highlight/true#M57323)

Todo: additional sanity checks in case your copy of gzip isn't the same as mine.

Todo: finish hash/checksum recalculation so patch tool actually works.

Until then, take the output of the current incomplete patch tool and add the
following patches:

* At `0x30` replace the MD5 with `79 D7 1E DF 3D B9 11 DB 8C B8 9B 6C 93 AA 4F 80`.
* At `0x90` and `0x210` replace the MD5 with `DA 54 1E CC 3E 2C 71 92 9B F9 99 EC 23 B9 39 84`.
* At `0xFF` and `0x27F` replace the checksum with `74`.

> Knowing that the programmer might one day return and finish the program...
> 
> It fills you with determination.
