#!/bin/sh

gcc -Wall -o ramcart2xex ramcart2xex.c
./ramcart2xex
gcc -Wall -o car2rom car2rom.c
./car2rom
gcc -Wall -o SectorMap SectorMap.c
./SectorMap
xxd -cols 16 -i SectorMap.dta > SectorMap.h

./mads starter.asm -o:starter.bin
xxd -cols 16 -i starter.bin > starter.h

gcc -Wall -o atr2ramcart atr2ramcart.c
./atr2ramcart base.atr base.car
./car2rom base.car base.bin
./ramcart2xex base.bin base.xex
./atr2ramcart

./atari800 base.car

