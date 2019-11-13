#!/bin/sh

BINFILE=my_game.bin

if [ -f $BINFILE ]; then
	rm $BINFILE
fi

echo "write from cc binary"
./writer
hexdump -C $BINFILE
echo

echo "read from python binary"
./reader.py
echo

echo "print using c binary"
./printer
echo
