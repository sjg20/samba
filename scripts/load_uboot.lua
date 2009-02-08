a = at91.open(0x3eb, 0x6124)

a:write_file(0x23f00000, "/home/andre/snapper/u-boot-1.1.6/u-boot.bin")
a:go(0x23f00000)

