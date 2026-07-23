# QEMU launch script
# 
#  Usage: qemu [cd/disk] [image] [debug]
# 
#  Examples: 
#     qemu.ps1 cd build/bee.iso 
#     qemu.ps1 cd build/bee.iso debug
#     qemu.ps1 disk build/bee.img debug 

if ($args.Length -lt 2) {
   Write-Host "Usage: qemu.ps1 <cd|disk> <image-path> [debug]"
   exit 1
}

$mediaType = $args[0]
$imagePath = $args[1]
$debugMode = $false

for ($i = 2; $i -lt $args.Length; $i++) {
   if ($args[$i] -eq "debug") {
      $debugMode = $true
   }
   else {
      Write-Host "Unknown option: $($args[$i])"
      exit 1
   }
}

$qemuArgs = @()
switch ($mediaType) {
   "cd"   { $qemuArgs += @("-cdrom", $imagePath) }
   "disk" { $qemuArgs += @("-drive", "format=raw,file=$imagePath") }
   default {
      Write-Host "Unknown media type: $mediaType (expected 'cd' or 'disk')"
      exit 1
   }
}

$qemuArgs += @(
   "-serial",     "stdio",
   "-debugcon",   "file:logs/debug-qemu.log",
   "-no-reboot",
   "-no-shutdown",
   "-d",          "int,cpu_reset",
   "-D",          "logs/qemu.log"
)

if ($debugMode) {
   $qemuArgs += @("-s", "-S")
   Write-Host "Debug mode: QEMU will wait on gdb at localhost:1234"
}

& \msys64\mingw64\bin\qemu-system-i386 @qemuArgs
