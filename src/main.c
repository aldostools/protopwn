// Stage 1 payload
// Executed by OSDSYS as the browser thread
#include <debug.h>
#include <kernel.h>
#include <ps2sdkapi.h>
#include <sifrpc.h>
#include <string.h>
#define NEWLIB_PORT_AWARE
#include <fileio.h>

// Reduce binary size by disabling the unneeded functionality
void _libcglue_init() {}
void _libcglue_deinit() {}
void _libcglue_args_parse(int argc, char **argv) {}
DISABLE_PATCHED_FUNCTIONS();
DISABLE_EXTRA_TIMERS_FUNCTIONS();
PS2_DISABLE_AUTOSTART_PTHREAD();

// Loader ELF variables
extern uint8_t loader_elf[];
extern int size_loader_elf;

// Kernel patches
extern uint8_t kpatch_0100_img[];
extern int size_kpatch_0100_img;

extern uint8_t kpatch_0101_img[];
extern int size_kpatch_0101_img;

// Applies the kernel patch
void applyKPatch(uint8_t *eeload, int eeload_size);
// Runs the target ELF using the embedded ELF loader
int LoadELFFromFile(int argc, char *argv[]);

// Deinitializes OSDSYS and executes the target ELF
// Both this function and the embedded ELF loader reuse memory card modules loaded by OSDSYS
int launch() {
  // Deinit OSDSYS

  // Disable interrupts
  DisableIntc(3);
  DisableIntc(2);

  // Disable DMAC channel 5 handler, important for getting ExecPS2 to work properly
  // Fails when not called from the OSDSYS main thread
  DisableDmacHandler(5);
  RemoveDmacHandler(5, 2);

  // Terminate all other threads
  uint32_t tID = GetThreadId();
  ChangeThreadPriority(tID, 0);
  CancelWakeupThread(tID);
  for (int i = 0; i < 0x100; i++) {
    if (i != tID) {
      TerminateThread(i);
      DeleteThread(i);
    }
  }

  // Reinitialize DMAC, VU0/1, VIF0/1, GIF, IPU
  ResetEE(0x7F);

  FlushCache(0);
  FlushCache(2);

  char *argv[1] = {
      "mc0:/" BOOT_PATH,
  };

  // Make sure the file exists on either mc0 or mc1
  int fd = fioOpen(argv[0], FIO_O_RDONLY);
  if (fd < 0) {
    argv[0][2] = '1';
    fd = fioOpen(argv[0], FIO_O_RDONLY);
  }
  if (fd < 0)
    return -1;

  fioClose(fd);

  LoadELFFromFile(1, argv);
  __builtin_trap();
}

int main() {
  DI();
  // Patch protokernel function to execute the launch function in the main thread
  if ((_lw(0x00204c88) & 0xfcff0000) == 0x0c080000) { // ROM 0100
    applyKPatch(kpatch_0100_img, size_kpatch_0100_img);
    _sw((0x0c000000 | ((uint32_t)launch >> 2)), (uint32_t)0x00204c88); // jal launch
  } else if ((_lw(0x00204ce0) & 0xfcff0000) == 0x0c080000) {           // ROM 0101
    applyKPatch(kpatch_0101_img, size_kpatch_0101_img);
    _sw((0x0c000000 | ((uint32_t)launch >> 2)), (uint32_t)0x00204ce0); // jal launch
  }
  EI();

  FlushCache(0);
  FlushCache(2);

  // Exit
  ExitDeleteThread();
  return 0;
}

// Applies the kernel patch
void applyKPatch(uint8_t *eeload, int eeload_size) {
  ee_kmode_enter();

  // Copy EELOAD image into kernel memory
  uint32_t i;
  volatile uint32_t *start;
  for (i = 0, start = (volatile uint32_t *)0x80030000; i < eeload_size / 4; i++) {
    start[i] = ((volatile uint32_t *)eeload)[i];
  }

  // Patch the start and the end of address range of the IOPRP image containing EELOAD.
  *(volatile uint16_t *)0x80005670 = 0x8003;
  *(volatile uint16_t *)0x80005674 = 0x8004;

  ee_kmode_exit();
}

typedef struct {
  uint8_t ident[16]; // struct definition for ELF object header
  uint16_t type;
  uint16_t machine;
  uint32_t version;
  uint32_t entry;
  uint32_t phoff;
  uint32_t shoff;
  uint32_t flags;
  uint16_t ehsize;
  uint16_t phentsize;
  uint16_t phnum;
  uint16_t shentsize;
  uint16_t shnum;
  uint16_t shstrndx;
} elf_header_t;

typedef struct {
  uint32_t type; // struct definition for ELF program section header
  uint32_t offset;
  void *vaddr;
  uint32_t paddr;
  uint32_t filesz;
  uint32_t memsz;
  uint32_t flags;
  uint32_t align;
} elf_pheader_t;

#define ELF_MAGIC 0x464c457f
#define ELF_PT_LOAD 1

// Runs the target ELF using the embedded ELF loader
// Based on PS2SDK elf-loader
int LoadELFFromFile(int argc, char *argv[]) {
  uint8_t *boot_elf;
  elf_header_t *eh;
  elf_pheader_t *eph;
  void *pdata;
  int i;

  // Wipe memory region where the ELF loader is going to be loaded (see loader/linkfile)
  memset((void *)0x00084000, 0, 0x00100000 - 0x00084000);

  boot_elf = (uint8_t *)loader_elf;
  eh = (elf_header_t *)boot_elf;
  if (_lw((uint32_t)&eh->ident) != ELF_MAGIC)
    __builtin_trap();

  eph = (elf_pheader_t *)(boot_elf + eh->phoff);

  // Scan through the ELF program headers and copy them into RAM
  for (i = 0; i < eh->phnum; i++) {
    if (eph[i].type != ELF_PT_LOAD)
      continue;

    pdata = (void *)(boot_elf + eph[i].offset);
    memcpy(eph[i].vaddr, pdata, eph[i].filesz);
  }

  sceSifExitCmd();
  sceSifExitRpc();
  FlushCache(0);
  FlushCache(2);

  return ExecPS2((void *)eh->entry, NULL, argc, argv);
}
