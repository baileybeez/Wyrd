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
dataImage=""

while [ "$#" -gt 0 ]; do
   case "$1" in
      debug)
         debugMode=true
         ;;
      data)
         shift
         dataImage="${1:-}"
         if [ -z "$dataImage" ]; then
            echo "Option 'data' requires an image path"
            exit 1
         fi
         ;;
      *)
         echo "Unknown option: $1"
         exit 1
         ;;
   esac
   shift
done

qemuArgs=()
case "$mediaType" in
   cd)
      qemuArgs+=(-cdrom "$imagePath" -boot d)
      ;;
   disk)
      qemuArgs+=(-drive "format=raw,file=$imagePath")
      ;;
   *)
      echo "Unknown media type: $mediaType (expected 'cd' or 'disk')"
      exit 1
      ;;
esac

if [ -n "$dataImage" ]; then
   qemuArgs+=(-drive "format=raw,file=$dataImage,if=ide,index=0,media=disk")
fi

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
