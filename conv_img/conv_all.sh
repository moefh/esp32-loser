#!/bin/bash

OUT_DIR="$1"

if [ -z "${OUT_DIR}" ]; then
    OUT_DIR=../vga_game
fi

SYNC_BITS=0xc0

echo "=== un-gzipping =========="
for file in spr/*.gz; do
  gunzip ${file}
done

echo "=== converting ==========="
for file in spr/*.spr; do
  ./conv_spr -sync ${SYNC_BITS} -num-frames 64 ${file}
done

echo "=== copying =============="
cp -v spr/*.h "${OUT_DIR}"
