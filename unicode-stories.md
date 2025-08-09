## Background

I found there is problem, the RetroArch can not scan roms directories and files that contain none-ascii characters on Switch.

A same issue here by [goooooouwa](https://github.com/goooooouwa):
[Filenames with Chinese characters (e.g. roms, thumbnails) are not recognized by Retroarch and Lakka for Nintendo Switch](https://github.com/libretro/RetroArch/issues/13424)

## the bug detail

1. @SciresM says it's a HOS(HorizonOS) core bug:
https://github.com/m4xw/emuMMC/issues/29

2. @hexkyz says the detail about the bug
  https://github.com/switchbrew/libnx/issues/90, I think it is the closest reason that almost all homebrew software author does not support unicode path names.

## my research about how retroarch failed to scan unicode paths

retroarch is using libnx library, however the libnx is using HOS kernel service called `fs-srv`. So any userland program or applet mode app relys on still cannot get the correct decoded filename.


### Begin journey: How retroarch calling to Libnx

retro_dirent.c
```c
int retro_readdir(struct RDIR *rdir)

  retro_vfs_readdir_impl
```

vfs_implementatiion.c
```c
libretro_vfs_implementation_dir *retro_vfs_opendir_impl(
      const char *name, bool include_hidden)

      ...
      opendir(name)
      ...
```

### Deep Search: How libnx calling to fs-srv

libnx : dirent.h
```c
struct dirent *
	 readdir(DIR *);
```
_syslist.h
```c
#define _readdir readdir
```

after sereval mappings by io_support.h, newlib.h and so on

fs_dev.c
```c
static const devoptab_t
fsdev_devoptab =
{
  .structSize   = sizeof(fsdev_file_t),
  .open_r       = fsdev_open,
  .close_r      = fsdev_close,
  .write_r      = fsdev_write,
  .read_r       = fsdev_read,
  .seek_r       = fsdev_seek,
  .fstat_r      = fsdev_fstat,
  .stat_r       = fsdev_stat,
  .link_r       = fsdev_link,
  .unlink_r     = fsdev_unlink,
  .chdir_r      = fsdev_chdir,
  .rename_r     = fsdev_rename,
  .mkdir_r      = fsdev_mkdir,
  .dirStateSize = sizeof(fsdev_dir_t),
  .diropen_r    = fsdev_diropen,
  .dirreset_r   = fsdev_dirreset,
  .dirnext_r    = fsdev_dirnext,
  .dirclose_r   = fsdev_dirclose,
  .statvfs_r    = fsdev_statvfs,
  .ftruncate_r  = fsdev_ftruncate,
  .fsync_r      = fsdev_fsync,
  .deviceData   = 0,
  .chmod_r      = fsdev_chmod,
  .fchmod_r     = fsdev_fchmod,
  .rmdir_r      = fsdev_rmdir,
  // symlinks aren't supported so alias lstat to stat
  .lstat_r      = fsdev_stat,
};
```

libnx : fs_dev.c
```c
static int
fsdev_open(struct _reent *r,
          void          *fileStruct,
          const char    *path,
          int           flags,
          int           mode)
{
  FsFile        fd;
  Result        rc;
  u32           fsdev_flags = 0;
  u32           attributes = 0;
  char         *fs_path = __nx_dev_path_buf;
  fsdev_fsdevice *device = r->deviceData;

  if(fsdev_getfspath(r, path, &device, fs_path)==-1)
    return -1;

  /* get pointer to our data */
  fsdev_file_t *file = (fsdev_file_t*)fileStruct;
  ...


```

libnx : fs.c
```c
Result fsFsOpenFile(FsFileSystem* fs, const char* path, u32 mode, FsFile* out) {
    return _fsFsOpenCommon(fs, path, mode, &out->s, 8);
}

... _fsObjectDispatchIn

... HIPC-CMIF
```

### HIPC  (Horizon Inter-Process Communication)
https://switchbrew.org/wiki/HIPC

HIPC (Horizon Inter-Process Communication) is a custom IPC implementation tailored for the Horizon OS.

There are two protocols over HIPC:

- CMIF: Command Interface (?). Supports session management with domains and dynamic multiplexing of services.
- TIPC: Tiny IPC. Simple and lightweight protocol.

### CMIF (Command Interface Framework)


[Detail source code of HIPC and CMIF](hipc-cmif.md)

### The suspicious part of Directory iteration

libnx: fs_dev.c

```c
static int
fsdev_dirnext(struct _reent *r,
             DIR_ITER      *dirState,
             char          *filename,
             struct stat   *filestat)
{             
    /* convert name from fs-path to UTF-8 */
    memset(filename, 0, NAME_MAX);
    units = fsdev_convertfromfspath((uint8_t*)filename, (uint8_t*)entry->name, NAME_MAX);
}
```

Actualy, the fsdev_convertfromfspath is not convert HIPC result to utf-8, it just copy the string.


```c
static ssize_t fsdev_convertfromfspath(uint8_t *out, uint8_t *in, size_t len)
{
  ssize_t inlen = strnlen((char*)in, len);
  memcpy(out, in, inlen);
  if (inlen < len)
    out[inlen+1] = 0;
  return inlen;
}
```

## What I have tried?

1. Port libiconv: successfully compiled, costs a lot of time.
port iconv to libnx, successfully compiled, installed, when I compile a simple libnx the sdmc example, compiled but crashed black screen.

By lacking of devkitPro toolset linking knownledge, this progress failed at the end. But I learned how to port libraries and how to use devkitPro toolset.

If someday I want to, I will try to port iconv to libnx, maybe I lost some Makefile arguments.

2. compile libnx: change `fsdev_convertfromfspath` function to use libiconv.
A lot of compile errors have been fixed, but launched with AtmosphereNX, as I said above, it still crashed black screen.

3. modifing libnx switch/fs/sdmc example program.
Just to print some Unicode value, it always print none-unicode characters to `_` underscore, `U+005F`, and reject to print directory with full unicode chars.

4. Find out who, where, how the Ams or libnx or some fs lib handle none-unicode to underscore.
The Answer is : The Fatfs without unicode. And something more I do not known.

It's difficult to verify or find out solution to fix or get rid of HOS own fs bug.

Here are some reason to prevent us:
https://github.com/switchbrew/libnx/issues/592#issuecomment-1179520564

Aparently, the libnx default console does not print Unicode characters. Because the default_font.bin only contains ASCII 1-bit characters.
 And the 'render' program is designed to support simple character mapping coordinates.

I can see a few person tried to print unicode characters on libnx console, from Gbatemp forum.

It's achieveable by programing our own console, or use SDL2\Webkit to render text. that is another topic.

5. avoid using libnx console, try payload
To get rid of HOS filesystem bug, that is not using libnx the service wrapper, I tried to compile payload based software `TegraExplorer`, but when I try to modify FatFs unicode support, TegraExplorer says Payload size exceeds limit. `:(`

6. Learn some DBCS knowledge from Microsoft and appreciate them.
I can see FatFs source code, it originally support DBCS such as Shift-JIS, but it's not enabled by libnx, and other homebrews.

[Character Sets Used in File Names](https://learn.microsoft.com/en-us/windows/win32/intl/character-sets-used-in-file-names)

7. Learn some homebrews such as DBI, Goldleaf to findout how they handle unicode.

The DBI (closedsource, author: rashevskyv ) handles chinese very well, do not know how does he achieved

The another one is Goldleaf (author: XorTroll), he made lots of software fundations even a GUI framework `Plutonium`, then we can see that he is using SDL2. So to render a graphic character (CJK and others) is difficult without SDL2 or OpenGL or Webkit.


## Conclusion
1. It turns out that the Fatfs library `FF_LFN_UNICODE=2` `FF_CODE_PAGE=850` `FF_USE_LFN=3` cannot solve the Path problem.
2. If you build TegraExplorer and print the FatFs read path, you will get right binary char buffer. That is because TegraExplorer is payload program, it do not use Hos services.
3. By accessing the libnx fs_dev.h the low-level fs functions such as `fsFsOpenDirectory`, `fsDirRead`, `fsDirGetEntryCount`, `fsDirGetEntry`, `fsDirClose` can prove that the Nintendo HOS Kernel service `fsp-srv` is the root problem.
4. By testing with TegraExplorer the default behavior of fatfs with `FF_CODE_PAGE=Latin1', it convert unicode character to '?'.