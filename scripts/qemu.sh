#!/usr/bin/env bash
#
#  Usage: qemu.sh [cd/disk] [image] [debug]
#
#  Examples:
#     qemu.sh cd build/wyrd.iso
#     qemu.sh cd build/wyrd.iso debug
#     qemu.sh disk build/wyrd.img debug

set -euo pipefail

qemuBin="${QEMU:-qemu-system-i386}"

if [ "$#" -lt 2 ]; then
   echo "Usage: qemu.sh <cd|disk> <image-path> [debug]"
   exit 1
fi

mediaType="$1"
imagePath="$2"
shift 2

debugMode=false

for arg in "$@"; do
   if [ "$arg" = "debug" ]; then
      debugMode=true
   else
      echo "Unknown option: $arg"
      exit 1
   fi
done

qemuArgs=()
case "$mediaType" in
   cd)
      qemuArgs+=(-cdrom "$imagePath")
      ;;
   disk)
      qemuArgs+=(-drive "format=raw,file=$imagePath")
      ;;
   *)
      echo "Unknown media type: $mediaType (expected 'cd' or 'disk')"
      exit 1
      ;;
esac

mkdir -p logs

qemuArgs+=(
   -serial     stdio
   -debugcon   file:logs/debug-qemu.log
   -no-reboot
   -no-shutdown
   -d          int,cpu_reset
   -D          logs/qemu.log
)

if [ "$debugMode" = true ]; then
   qemuArgs+=(-s -S)
   echo "Debug mode: QEMU will wait on gdb at localhost:1234"
fi

exec "$qemuBin" "${qemuArgs[@]}"
