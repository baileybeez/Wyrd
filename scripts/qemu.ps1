\msys64\mingw64\bin\qemu-system-x86_64 `
   -cdrom build/bee.iso `
   -serial stdio `
   -debugcon file:logs/debug-qemu.log `
   -no-reboot `
   -no-shutdown `
   -d int,cpu_reset `
   -D logs/qemu.log
