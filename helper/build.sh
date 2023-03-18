#!/bin/bash

# sudo apt install gcc-mipsel-linux-gnu

JOBS=`grep -c ^processor /proc/cpuinfo`
mkdir -p ./build-output ./build-hatlab_gateboard-one

export CROSS_COMPILE=mipsel-linux-gnu-
cp ./mt7621_stage_sram.blob ./build-hatlab_gateboard-one/mt7621_stage_sram.bin

make O=build-hatlab_gateboard-one hatlab_gateboard_one_880_defconfig
make O=build-hatlab_gateboard-one -j${JOBS}
cp build-hatlab_gateboard-one/u-boot-mt7621.bin ./build-output/hatlab_gateboard-one-bios.880MHz.bin

make O=build-hatlab_gateboard-one hatlab_gateboard_one_1000_defconfig
make O=build-hatlab_gateboard-one -j${JOBS}
cp build-hatlab_gateboard-one/u-boot-mt7621.bin ./build-output/hatlab_gateboard-one-bios.1000MHz.bin

make O=build-hatlab_gateboard-one hatlab_gateboard_one_1200_defconfig
make O=build-hatlab_gateboard-one -j${JOBS}
cp build-hatlab_gateboard-one/u-boot-mt7621.bin ./build-output/hatlab_gateboard-one-bios.1200MHz.bin
