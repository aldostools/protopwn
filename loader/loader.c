/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# (c) 2020 Francisco Javier Trujillo Mata <fjtrujy@gmail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/

#include <iopcontrol.h>
#include <iopheap.h>
#include <kernel.h>
#include <loadfile.h>
#include <ps2sdkapi.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdlib.h>
#include <string.h>

//--------------------------------------------------------------
// Redefinition of init/deinit libc:
//--------------------------------------------------------------
// DON'T REMOVE is for reducing binary size.
// These functions are defined as weak in /libc/src/init.c
//--------------------------------------------------------------
void _libcglue_init() {}
void _libcglue_deinit() {}
void _libcglue_args_parse(int argc, char **argv) {}

DISABLE_PATCHED_FUNCTIONS();
DISABLE_EXTRA_TIMERS_FUNCTIONS();
PS2_DISABLE_AUTOSTART_PTHREAD();

//--------------------------------------------------------------
// Start of function code:
//--------------------------------------------------------------
// Clear user memory
// PS2Link (C) 2003 Tord Lindstrom (pukko@home.se)
//         (C) 2003 adresd (adresd_ps2dev@yahoo.com)
//--------------------------------------------------------------
static void wipeUserMem(void) {
  for (int i = 0x100000; i < 0x02000000; i += 64) {
    asm volatile("\tsq $0, 0(%0) \n"
                 "\tsq $0, 16(%0) \n"
                 "\tsq $0, 32(%0) \n"
                 "\tsq $0, 48(%0) \n" ::"r"(i));
  }
}

int main(int argc, char *argv[]) {
  if (argc < 1) {
    return -EINVAL;
  }

  // Wipe the user memory
  wipeUserMem();

  // Apply the patch required to load executables from memory cards
  // (memory card modules are already loaded by OSDSYS)
  int ret;
  sceSifInitRpc(0);
  if ((ret = sbv_patch_disable_prefix_check()))
    return ret;

  // Writeback data cache before loading the ELF.
  FlushCache(0);

  // Load the ELF into memory
  static t_ExecData elfdata;
  elfdata.epc = 0;
  SifLoadFileInit();
  ret = SifLoadElf(argv[0], &elfdata);
  SifLoadFileExit();

  // Reboot the IOP
  while (!SifIopReset("", 0)) {
  };
  while (!SifIopSync()) {
  };

  if (ret == 0 && elfdata.epc != 0) {
    // Execute the ELF
    FlushCache(0);
    FlushCache(2);
    return ExecPS2((void *)elfdata.epc, (void *)elfdata.gp, argc, argv);
  } else {
    return -ENOENT;
  }
}
