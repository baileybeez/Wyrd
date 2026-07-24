# Wyrd

<p align="center" style='background: #e3dac9; color: #444; font-family: Garamond; font-size: 16pt;'>
   <img src='./wyrd.png' alt='Wyrd Logo' width='240'><br />
   <i>gæð a wyrd swa hio scel</i>
</p>

> [!WARNING]
> This software is purely for educational purposes and far from complete. It will crash and is missing key features. Keep your expectations low.

## Summary
Wyrd is a 32-bit operating system for x86 devices. The project goal is learning core concepts required to build and maintain a from-scratch operating system in ASM and C. It is a very barebones product, offering a serial console (for debugging), vga text support, basic FAT16 support via ATA, keyboard support, memory mapping, and virtual paging. 

---

## Getting Started

### Quickstart
```bash
$ ./make toolchain
$ ./make wyrd
```

### **32-bit x86 Toolchain**
Building Wyrd requires a 32bit x86 cross compiler. Running `./make toolchain` will download the necessary source code (`binutils`, `gcc`, and `gdb`), extract, and build them into `/toolchain/i686-elf`. These projects have the following requirements: `build-essential`, `bison`, `flex`, `libgmp-dev`, `libmpc-dev`, `libmpfr-dev`, `libisl-dev`, `libreadline-dev`, `libexpat1-dev`, `texinfo`, `wget`, `ca-certificates`, `xorriso`, `grub-pc-bin`, `grub-common`, `mtools`, `nasm`, `file`, `make`, `sudo`, `bsdmainutils`.
```bash
$ ./make toolchain
```

### **Building the kernel / disk**
Wyrd can be built into a disk image of two different flavors.

#### 1. GRUB
Leans on GRUB to handle all of the boot processes and handoff to the kernel. This will produce `build/wyrd.iso`.
```bash
$ ./make grub-iso
```

#### 2. Custom Bootload
Uses a custom bootloader to manage the entire boot process and handoff to the kernel. This will produce `build/wyrd.img`.
```bash
$ ./make wyrd-iso
```

### **Running via QEMU**
While any virtual machine should suffice in running/testing Wyrd, development and testing has been done primarly against QEMU. QEMU can be launched via: `scripts/qemu <cd|disk> <image> [debug]`

|  |  |
|-------|------|
|*cd*   |  boots as a cdrom image |
|*drive*|  boots as a disk image  |
|*image*|  the path to the image (i.e. `build/)wyrd.img`)   |
|*debug*|  (optional) enables QEMU's debug flags (`-s -S`) |

---

## Roadmap / Milestones

1. <s>"Hello, world" from kernel via GRUB Multiboot2 → VGA text buffer</s>
2. <s>Serial output working (debug lifeline)</s>
3. <s>GDT + IDT set up; exception handlers print register state instead of triple-faulting</s>
4. <s>Timer interrupt fires and increments a counter</s>
5. <s>PS/2 keyboard input echoed to screen</s>
6. <s>Physical page frame allocator (parse Multiboot memory map)</s>
7. <s>Paging enabled, kernel in higher-half, kmalloc/kfree</s>
8. <s>Custom Bootloader *Stage 1* 512-byte MBR that loads stage 2 from disk.</s>
9. <s>Custom Bootloader *Stage 2* loads kernel from disk, init GDT, memory via E820, A20, protected mode</s>
10. Two kernel threads visibly interleaving (scheduler, threads, and context switching)
11. Ring 3 transition, first userspace program printing via syscall
12. ATA PIO disk driver
13. FAT16 read-only filesystem driver
14. ELF loader, first program loaded from disk executed in userspace
15. In-kernel shell running programs from disk
