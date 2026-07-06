qemu `
   -cdrom build/bee.iso `
   -serial stdio `
   -no-reboot `
   -no-shutdown `
   -d int,cpu_reset `
   -D logs/qemu.log
