## Analysis of EmuMMC and sd card file system

EmuMMC is a virtual MMC storage device driver, it uses FatFs's ability to provide MMC logical file system.

It does not provide API to user applications, it only provide api to Atmosphere (used by Libstratosphere)

### How does EmuMMC connect to FatFS?

Take a look at fatfs source code, these prototype functions are used to access the storage device under the FatFS.

diskio.h (FatFs)
```c
/*---------------------------------------*/
/* Prototypes for disk control functions */
DSTATUS disk_initialize (BYTE pdrv);
DSTATUS disk_status (BYTE pdrv);
DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff);
```

### How does FatFs work? what role does it play?

FatFs supports file system on storage device, such as FAT12, FAT16, FAT32, exFAT

FatFs defines data structures: File Object, Directory Object, File Information, etc.

FatFs has four types of domain operations:
1. Directory Navigation
  - f_chdir() 
  - f_chdrive()
  - f_getcwd()
2. Volume Management
  - f_mount()
  - f_mkfs()
  - f_disk()
  - f_getfree()
  - f_getlabel() / f_setlabel()
3. Directory Operations  
  - f_opendir()
  - f_readdir()
  - f_findnext()
  - f_mkdir()
4. File Operations
  - f_open() / f_close()
  - f_read() / f_write()
  - f_lseek() / f_truncate()
  - f_sync()

Note: the introduction is simple, details are omitted. the details can be found in the source code.

### The DiskIO layer (interface)
It's obvious that FatFs is a logical FileSystem, it needs a physical disk driver to access the storage controller.

To achieve that, the Emummc plays a driver role on it. Emummc implements the diskio.h interface, and register it to FatFs. Emummc do the actually disk io even power detection.

for example the core functions are: disk_read and disk_write, etc. See the previous 'prototype' functions.

### how does emummc plug into FatFs's disk_read or disk_write

see the source code of 'sdmmc.h' (emuMMC)
```c
int sdmmc_storage_read(sdmmc_storage_t *storage, u32 sector, u32 num_sectors, void *buf);
int sdmmc_storage_write(sdmmc_storage_t *storage, u32 sector, u32 num_sectors, void *buf);
```
For example, the disk_read function is implemented as follows:
```c
/**sdmmc layer the glue between FatFs and emummc **/
static int _sdmmc_storage_readwrite(sdmmc_storage_t *storage, u32 sector, u32 num_sectors, void *buf, u32 is_write)

static int _sdmmc_storage_readwrite_ex(sdmmc_storage_t *storage, u32 *blkcnt_out, u32 sector, u32 num_sectors, void *buf, u32 is_write)

int  sdmmc_execute_cmd(sdmmc_t *sdmmc, sdmmc_cmd_t *cmd, sdmmc_req_t *req, u32 *blkcnt_out);

/**sdmmc_driver.c -> hardware driver layer**/
static int _sdmmc_execute_cmd_inner(sdmmc_t *sdmmc, sdmmc_cmd_t *cmd, sdmmc_req_t *req, u32 *blkcnt_out);

static int _sdmmc_send_cmd(sdmmc_t *sdmmc, sdmmc_cmd_t *cmd, bool is_data_present) {
    /**omited other code**/
    //Write to the hardware address (maybe the register)
    sdmmc->regs->argument = cmd->arg;
	sdmmc->regs->cmdreg = (cmd->cmd << 8) | cmdflags;
}
```

### the DMA address binding 

```c
int sdmmc_init(sdmmc_t *sdmmc, u32 id, u32 power, u32 bus_width, u32 type, int powersave_enable)
{
    /*etc...*/
	sdmmc->regs = (t210_sdmmc_t *)QueryIoMapping(_sdmmc_bases[id], 0x200);
}

static int _sdmmc_config_dma(sdmmc_t *sdmmc, u32 *blkcnt_out, sdmmc_req_t *req) {
   /**omited other code**/
    sdmmc->regs->admaaddr = admaaddr & 0xFFFFFFFFF;
    sdmmc->regs->admaaddr_hi = (admaaddr >> 32) & 0xFFFFFFFFF;
}
```

### How does the EmuMMC self-infesting work?
```c
//emummc.h
bool sdmmc_initialize(void);

called by _sdmmc_ensure_initialized()

called by sdmmc_wrapper_read

defined by setup_hooks();

 // sdmmc_wrapper_read hook
    INJECT_HOOK(fs_offsets->sdmmc_wrapper_read, sdmmc_wrapper_read);
    // sdmmc_wrapper_write hook
    INJECT_HOOK(fs_offsets->sdmmc_wrapper_write, sdmmc_wrapper_write);

called by main.c
void __init();
void __initheap(void);

called by start.s (ASM)
__initheap and __init
```

### How does Emummc treat the different paths?

It redirects the Emmc Read/Write requests to SDcard from /Nintendo to /emummc/Nintendo_XXXX (File-Based, or Raw Partition)

So, it is exactly that what they said:
> Full support for `/Nintendo` folder redirection to a arbitrary path 

main.c
```c
// Nintendo Path
static char nintendo_path[0x80] = "Nintendo";

static void load_emummc_ctx(void) {
    /**/
    memcpy(nintendo_path, paths.nintendo_path, sizeof(nintendo_path) - 1);
    intendo_path[sizeof(nintendo_path) - 1] = 0;
    if (strcmp(nintendo_path, "") == 0)
    {
        snprintf(nintendo_path, sizeof(nintendo_path), "emummc/Nintendo_%04x", emuMMC_ctx.id);
    }
}

```

### How does Emummc support 'No 8 char length restriction!'?
back to FATFS, the ffconf.h has serveral options related to the LFN and Unicode support.

```c
#define FF_USE_LFN		3
/* The FF_USE_LFN switches the support for LFN (long file name).
/
/   0: Disable LFN. FF_MAX_LFN has no effect.
/   1: Enable LFN with static working buffer on the BSS. Always NOT thread-safe.
/   2: Enable LFN with dynamic working buffer on the STACK.
/   3: Enable LFN with dynamic working buffer on the HEAP.
```

### Conclusion
The Emummc is really a hardcore filesystem driver. 

How ever it may not solve the Unicode Path Problem.

Because it is forward the /Nintendo paths passively. 

If user application access the Hos Kernel service `Fsp-srv`, except the /Nintendo paths use FatFs (may decode Path correctly if configured codepage well), other paths will still access the filesystem provided by `OpenSdCardFileSystem`. The emummc may only forward the Emummc read/write request issued by the kernel. 

Maybe writing directories and files into Emummc file-based/raw partition then read it from Emummc, use may get the right path.
But the reader is still the kernel at this moment.

Applications normally use stdio (POSIX) or libnx api to access the SdCard, for example, the RetroArch use libnx to browse sd card, it never ask kernel to access MMC (The kernel thought), so it is impossible to get the right unicode path data so far.