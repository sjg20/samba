-- Example at91 script
local addr

printf = function(s,...)
    return io.write(s:format(...))
end -- function


a = at91.open(0x3eb, 0x6124)
v = a:version()
print ("Version: ".. v)
a:dbg_init(115200)
a:dbg_print("AT91 samba testing script\r\n");
for addr = 0x20000000, 0x23ffffff, 0x00100000 do
    a:writel(addr, addr)
end
for addr = 0x20000000, 0x23ffffff, 0x00100000 do
    if not (a:readl(addr) == addr) then
        printf("SDRAM fails @ 0x%8.8x (0x%8.8x != 0x%8.8x)\n",
                    addr, a:readl(addr), addr)
    else
        printf("SDRAM good @ 0x%8.8x (0x%8.8x)\n", addr, a:readl(addr))
    end
end

--a:write_file(0x200000, "/home/andre/snapper/Bootstrap-v1.9/board/sn9260/sn9260_bootstrap.bin")
--a:go(0x200000)

--a:nand_read_file(0x20000, "/home/andre/u-boot.bin", 0x40000)
a:nand_erase(0x0, 0x20000)
a:nand_write_file(0x0, "/home/andre/snapper/Bootstrap-v1.9/board/sn9260/sn9260_bootstrap.bin")
-- a.close ()

--a:write_file(0x23f00000, "/home/andre/snapper/u-boot-1.1.6/u-boot.bin")
--a:verify_file(0x23f00000, "/home/andre/snapper/u-boot-1.1.6/u-boot.bin")
--a:read_file(0x23f00000, "/home/andre/output.bin", 256000);
--a:go(0x23f00000)

