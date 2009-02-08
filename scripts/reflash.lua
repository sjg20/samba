a = at91.open(0x3eb, 0x6124)
v = a:version()
print ("Version: " .. v)
a:nand_erase(0x0, 0x60000)
a:nand_write_file(0x0, "/home/andre/snapper/Bootstrap-v1.9/board/sn9260/sn9260_bootstrap.bin")
a:nand_write_file(0x20000, "/home/andre/snapper/u-boot-1.1.6/u-boot.bin")
a:go(0x10000) -- start the standard rom boot
