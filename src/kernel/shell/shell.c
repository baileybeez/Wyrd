#include "wyrd.h"
#include "shell.h"
#include "arch/i686/ticks.h"
#include "drivers/input/keyboard.h"
#include "drivers/serial/serial.h"
#include "drivers/video/vga.h"
#include "fs/vfs/vfs.h"
#include "lib/logger.h"
#include "lib/mem.h"
#include "mm/heap.h"
#include "mm/pmm.h"
#include "scheduler/scheduler.h"
#include "scheduler/thread.h"
#include "string.h"

#define kBackspace    '\b'

#define kShellMaxLine 256
#define kMaxArgs      8

#define kShellCmd_Invalid  0
#define kShellCmd_ls       1
#define kShellCmd_cd       2
#define kShellCmd_meminfo  3
#define kShellCmd_ps       4
#define kShellCmd_clear    5
#define kShellCmd_uptime   6
#define kShellCmd_cat      7
#define kShellCmd_pwd      8

#define kShellCmd_Count    9

#define kShellRootFolder   "/"

typedef i32 (*ShellCmd)(u32, char**);

typedef struct {
   char  buffer[kShellMaxLine];
   char* args[kMaxArgs + 1];
   u32   argc;
} CmdLine;

typedef struct {
   char     name[kShellMaxLine];
   ShellCmd fnc;
} ShellBuiltin;

static Thread* _thread = nil;
static char    _cwd[kMaxPath];

static i32 _shellCmd_ls(u32 argc, char** argv)
{
   Dir      dir   = {0};
   DirEntry entry = {0};
   VfsError err;
   
   err = vfsDirOpen(_cwd, &dir);
   if (err != kVfsErr_OK)
      return 1;

   while ((err = vfsDirRead(&dir, &entry)) == kVfsErr_OK) {
      printf("%s\n", entry.name);
   }
   vfsDirClose(&dir);

   return 0;
}

static void _shellCwdPop(void)
{
   u32 len = strlen(_cwd);
   if (len <= 1) {
      return;
   }

   u32 i = len - 1;
   while (i > 0 && _cwd[i - 1] != '/') {
      i--;
   }
   _cwd[i] = '\0';
}

static i32 _shellCmd_pwd(u32 argc, char** argv)
{
   printf(_cwd);
   printf("\n");
   return 0;
}

static i32 _shellCmd_cd(u32 argc, char** argv)
{
   if (argc < 2) {
      strcpy(_cwd, kShellRootFolder);
      return 0;
   }
   
   char tmp[kMaxPath];
   if (!vfsResolvePath(_cwd, argv[1], tmp, kMaxPath)) {
      printf("dir not found!");
      return 1;
   }

   if (!vfsIsDirectory(tmp)) {
      printf("dir not found!");
      return 2;
   }

   strcpy(_cwd, tmp);
   return 0;
}

static const char* _sizeSuffix[5] = { "", "KB", "MB", "GB", "TB" };
static i32 _shellCmd_meminfo(u32 argc, char** argv)
{
   u32 v = heapFreeBytes();
   u32 i = 0;
   while (v > 1024 && i < 4) {
      i++;
      v /= 1024;
   }

   printf("meminfo: free frames= %u free heap= %u %s\n", pmmFreeFrameCount(), v, _sizeSuffix[i]);
   return 0;
}

static i32 _shellCmd_ps(u32 argc, char** argv)
{
   kUnused(argc);
   kUnused(argv);
   
   ProcessInfo info[kMaxThreads];
   u32 count = 0;

   printf("%6s %18s %6s %6s %s\n", "ID", "NAME", "PARENT", "STATE", "EXIT");
   printf("------ ------------------ ------ ------ ----\n");

   count = threadRegistrySnapshot(info, kMaxThreads);
   for (u32 i = 0; i < count; i++) {
      printf("%6u %18s %6u %6s %4i\n", 
         info[i].id, info[i].name, info[i].parentId, 
         threadStateName(info[i].state), info[i].exitCode);
   }

   return 0;
}

static i32 _shellCmd_clear(u32 argc, char** argv)
{
   kUnused(argc);
   kUnused(argv);
   vgaClear();
   return 0;
}

static i32 _shellCmd_uptime(u32 argc, char** argv)
{
   kUnused(argc);
   kUnused(argv);
   
   u32 u = ticksGetCount() / ticksGetHz();

   u32 s = u % 60;
   u /= 60;

   u32 m = u % 60;
   u /= 60;

   u32 h = u;

   printf("system uptime: %u h, %u m, %u s\n", h, m, s);
   return 0;
}

static i32 _shellCmd_cat(u32 argc, char** argv)
{
   if (argc < 2) {
      printf("usage: cat path/to/file\n");
      return 0;
   }

   char tmp[kMaxPath];
   if (!vfsResolvePath(_cwd, argv[1], tmp, kMaxPath)) {
      printf("file not found!");
      return 1;
   }

   // fd = openFile(tmp);
   // read = readFile(fd, buff, len);
   // print buff to output
   // loop until file read completely
}

static const ShellBuiltin kShellCommands[kShellCmd_Count] =
{
   [kShellCmd_Invalid]     = { .name = "",         .fnc = nil },
   [kShellCmd_ls]          = { .name = "ls",       .fnc = _shellCmd_ls },
   [kShellCmd_cd]          = { .name = "cd",       .fnc = _shellCmd_cd },
   [kShellCmd_meminfo]     = { .name = "meminfo",  .fnc = _shellCmd_meminfo },
   [kShellCmd_ps]          = { .name = "ps",       .fnc = _shellCmd_ps }, 
   [kShellCmd_clear]       = { .name = "clear",    .fnc = _shellCmd_clear },
   [kShellCmd_uptime]      = { .name = "uptime",   .fnc = _shellCmd_uptime },
   [kShellCmd_cat]         = { .name = "cat",      .fnc = _shellCmd_cat },
   [kShellCmd_pwd]         = { .name = "pwd",      .fnc = _shellCmd_pwd },
};

static void _shellEcho(char c)
{
   putChar(c);
   serialWriteChar(c);
   if (c == kBackspace) {
      serialWriteChar(' ');
      serialWriteChar(c);
   }
}

static void _shellReadLine(char outLine[kShellMaxLine], u32* outLen)
{
   u32 len     = 0;
   bool show   = false;
   bool ret    = false;
   KeyEvent ev = {0};
   while (true) {
      ev   = keyboardReadKey();
      show = false;
      if (ev.modifiers & (kMod_Ctrl | kMod_Alt))
         continue;

      if (ev.scanCode == kScanCode_Return) {
         outLine[len] = '\0';
         show = true;
         ret  = true;
      } else if (ev.scanCode == kScanCode_Backspace) {
         if (len > 0) {
            len--;
            show = true;
         }
      } else if (len + 1 < kShellMaxLine) {
         outLine[len++] = ev.ascii;
         show = true;
      }

      if (show) 
         _shellEcho(ev.ascii);
      
      if (ret) {
         *outLen = len;
         return;
      }
   }
}

// split apart the cmdline into 'cmd', 'argc', 'argv'
// + collapses multiple spaces between args
// - currently no support for quoted params or escaping
static void _shellParseLine(const char* line, u32 len, CmdLine* outCmd)
{
   if (len >= kShellMaxLine) {
      len = kShellMaxLine - 1;
   }
   memcpy(outCmd->buffer, line, len);
   outCmd->buffer[len] = '\0';

   char* p = outCmd->buffer;
   char* end = outCmd->buffer + len;
   while (p < end && outCmd->argc < kMaxArgs) {
      while (p < end && *p == ' ') {
         *p++ = '\0';
      }
      if (p == end) {
         break;
      }

      outCmd->args[outCmd->argc++] = p;
      while (p < end && *p != ' ') {
         p++;
      }
   }

   outCmd->args[outCmd->argc] = nil;
}

static ShellCmd _shellLookupInternalCmd(const char* cmd)
{
   for (u32 i = 0; i < kShellCmd_Count; i++) {
      if (strcmp(cmd, kShellCommands[i].name) == 0)
         return kShellCommands[i].fnc;
   }

   return nil;
}

static void _shellDispatchLine(const char* line, u32 len)
{
   CmdLine cmdLine = {0};
   _shellParseLine(line, len, &cmdLine);
   if (cmdLine.argc == 0)
      return;
   
   ShellCmd fnc = _shellLookupInternalCmd(cmdLine.buffer);
   if (fnc != nil) {
      fnc(cmdLine.argc, cmdLine.args);
   } else {
      // lookup elf in cwd
      // lookup elf in 'bin' dir
      // else -> print error msg
   } 
   // printf("%s: ", cmdLine.cmd);
   // for (u32 i = 0; i < cmdLine.argc; i++) {
   //    printf(cmdLine.args[i]);
   //    if (i + 1 < cmdLine.argc)
   //       printf(", ");
   // }   
   // putChar('\n');
}

static void _shellThread(void) 
{
   char line[kShellMaxLine] = {0};
   u32  len  = 0;

   kForever {
      print("> ");
      _shellReadLine(line, &len);
      if (len > 0) 
         _shellDispatchLine(line, len);
      
      kTrace("[kernel] ticks=%u idle=%u", ticksGetCount(), schedulerIdleCount());
   }
}

bool shellInit(void)
{
   memset(_cwd, 0x00, kMaxPath);
   strcpy(_cwd, kShellRootFolder);
   _thread = threadCreate(_shellThread, "shell");
   return _thread != nil;
}
