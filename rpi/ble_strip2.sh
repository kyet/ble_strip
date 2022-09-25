#!/bin/bash

DEV_ADDR="00:15:83:00:43:88"
MODE=$1
BRIGHTNESS=$2

# data-type len effect brightness fps param1~7

# common parameters
# brightness: 255
# fps: 30 (0x1e) (except aurora) 

if [ "$MODE" == "off" ]; then
	# 03 05 00 ff 1e
	cmd="03030503ff1e00"
elif [ "$MODE" == "daylight" ]; then
	# 03 05 01 ff 1e
	cmd="06030501ff1e00"
elif [ "$MODE" == "chromo" ]; then
	# 03 08 02 ff 1e e0 80 40
	# (hue1, hue2, hue3) = (pink 224, aqua 128, yellow 64)
	cmd="09030802ff1ee0804000"
elif [ "$MODE" == "rainbow" ]; then
	# 03 07 03 ff 1e 32 00
	# huefps: 50 (0x32)
	# cross: false (0x00)
	cmd="07030703ff1e0100"
elif [ "$MODE" == "rainbow_glitter" ]; then
	# 03 08 04 ff 1e 32 00 80
	# huefps: 50 (0x32)
	# cross: false (0x00)
	# chance: 50% (0x80) 
	cmd="07030804ff1e32028000"
elif [ "$MODE" == "confetti" ]; then
	# 03 07 05 ff 1e 32 00
	# huefps: 50 (0x32)
	# chance: 0
	cmd="07030705ff1e0100"
elif [ "$MODE" == "aurora" ]; then
	# 03 08 06 ff 14 14 C3 28
	# huefps: 20 (0x14)
	# hue:   195 (0xC3) - pink
	# range:  40 (0x28)
	cmd="09030806ff1414c32800"
elif [ "$MODE" == "aurora2" ]; then
	# 03 08 06 ff 14 14 64 28
	# huefps: 20 (0x14)
	# hue:   100 (0x64) - aqua
	# range:  40 (0x28)
	cmd="09030806ff1414642800"
elif [ "$MODE" == "get" ]; then
	# 00 02
	cmd="01020200"
else
	exit
fi

expect << EOF
spawn gatttool -b $DEV_ADDR -I 
send "connect\n" 
expect "Connection successful" 
expect ">" 
send "char-write-cmd 0x25 $cmd\n" 
expect ">" 
send "char-write-cmd 0x25 $cmd\n" 
expect ">" 
send "char-write-cmd 0x25 $cmd\n" 
expect ">" 
send "char-write-cmd 0x25 $cmd\n" 
expect ">" 
send "char-write-cmd 0x25 $cmd\n" 
expect ">" 
exit
EOF
