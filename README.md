# samba-script

AT91SAM9xxx SAM-BA control software for factory programming.


# Confirming the connection
To use samba-script, the AT91 device has to be put into SAM-BA mode. This is typically done by 
ensuring that there is no other valid boot device. On the Snapper 9260 QSK boards, this can be
done by holding down the SAMBA button whilst simultaneously pressing/releaseing the reset button.

Once in SAM-BA mode, the device should show up on USB. To check on a Linux system, use lsusb:
```sh
andre@teanau:~$ lsusb 
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 002 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 003 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 001 Device 086: ID 0403:ffa8 Future Technology Devices International, Ltd 
*Bus 001 Device 091: ID 03eb:6124 Atmel Corp. at91sam SAMBA bootloader*
```

# Commands
samba-script supports commands entered either interactively, or from a script. The following commands are supported:
* open [boardname]
* close
* version
* go [addr]
* readb [addr]
* readw [addr]
* readl [addr]
* read_file [addr] [filename] [length]
* verify_file [addr] [filename]
* writeb [addr] [val]
* writew [addr] [val]
* writel [addr] [val]
* write_file [addr] [file]
* debug_init [baudrate]
* debug_print [args...]
* nand_id
* nand_read_file [addr] [file] [length]
* nand_erase [addr] [length]
* nand_write_file [addr] [file]
* nand_write_raw_file [addr] [file]
* environment_init
* environment_save [addr]
* environment_variable [variable] [value]
* environment_file [file]
* print [args...]
* yaffs2 [file] [directory]

## Example command script
```
open sn9260
print  Erasing NAND
nand_erase 0x0 0x80000

print Writing Boot files
nand_write_file 0x0 /path/to/sn9260_bootstrap.bin
nand_write_file 0x20000 /path/to/u-boot.bin
```
