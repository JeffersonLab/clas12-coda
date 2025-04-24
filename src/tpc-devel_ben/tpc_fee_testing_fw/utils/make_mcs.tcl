if {$argc != 2} {
    puts "Need bitfile and output file name"
    exit
}
set bitfile_in [lindex $argv 0]
set mcsfile_out [lindex $argv 1]

write_cfgmem -format mcs -interface SPIx4 -size 256 -loadbit "up 0x0 $bitfile_in" -file $mcsfile_out -force
exit
