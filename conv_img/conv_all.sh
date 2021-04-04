#!/bin/bash

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
cp -v spr/*.h ../vga_game
