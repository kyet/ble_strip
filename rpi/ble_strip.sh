#!/bin/bash

DEV_ADDR="00:15:83:00:6E:F7"
MODE=$1
BRIGHTNESS=$2

# dev-type len effect brightness fps param1~3

if [ "$MODE" == "daylight" ]; then
	# 03 08 01 ff 1e 32 00 00
	cmd="07030801ff1e02010100"
elif [ "$MODE" == "chromo" ]; then
	# 03 08 02 ff 1e 32 00 00
	cmd="07030802ff1e02010100"
elif [ "$MODE" == "rainbow" ]; then
	# 03 08 03 ff 1e 32 00 00
	cmd="07030803ff1e02010100"
elif [ "$MODE" == "rainbow_glitter" ]; then
	# 03 08 04 ff 1e 32 00 00
	cmd="07030804ff1e02010100"
elif [ "$MODE" == "confetti" ]; then
	# 03 08 05 ff 1e 32 00 00
	cmd="07030805ff1e02010100"
elif [ "$MODE" == "chrom2" ]; then
	# 03 08 06 ff 1e 32 00 00
	cmd="07030806ff1e02010100"
elif [ "$MODE" == "rainbow2" ]; then
	# 03 08 07 ff 1e 32 00 00
	cmd="07030807ff1e02010100"
elif [ "$MODE" == "rainbow_glitter2" ]; then
	# 03 08 08 ff 1e 32 00 00
	cmd="07030808ff1e02010100"
elif [ "$MODE" == "off" ]; then
	# 03 08 00 ff 1e 32 00 00
	cmd="03030804ff1e02010100"
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
exit
EOF
