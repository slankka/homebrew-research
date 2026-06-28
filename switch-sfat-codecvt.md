# Switch SFAT codecvt notes

Date: 2026-06-28

This note summarizes the current understanding of the Nintendo Switch SD-card filename problem for non-ASCII paths. It updates the older hypothesis that the issue was mainly a libnx, homebrew FatFs, or rendering problem.

## Short answer

The problem is inside Horizon OS FS, not RetroArch and not libnx.

For the SD card FAT filesystem path, FS appears to use an embedded eSOL/Tuxera prFile-derived FAT driver. The path conversion layer is wired through a `PF_CHARCODE`-like table, but Nintendo's build uses CP932 / Shift-JIS conversion functions as the FAT filename codec.

That means standard FAT long filenames created by a PC are already stored correctly as UTF-16 on disk, but when Horizon FS converts those UTF-16 names back to application-visible path bytes, characters outside CP932 cannot round-trip. They are commonly replaced with `_` or become unopenable.

## Important distinction

There are two different representations that must not be confused.

PC-standard FAT LFN representation:

```text
name "卡带"
FAT LFN stores: U+5361 U+5E26
```

Current simple Switch patch representation:

```text
application path bytes: E5 8D A1 E5 B8 A6  (UTF-8 for "卡带")
FAT LFN stores:        U+00E5 U+008D U+00A1 U+00E5 U+00B8 U+00A6
```

The simple patch preserves UTF-8 bytes by storing each byte as a low `u16`. This is useful for Switch-created files, but it is not the same as standard UTF-16 long filenames written by Windows, Linux, macOS, cameras, phones, or normal FAT tools.

Therefore:

- The simple two-instruction patch can make Switch-created UTF-8 paths round-trip.
- It does not make PC-created standard UTF-16 FAT names readable.
- Full PC compatibility requires a real UTF-8 <-> UTF-16 conversion layer inside or beside FS.

## Evidence chain

The current evidence points to this flow:

```text
homebrew / application
  -> libnx fs_dev path bytes, normally UTF-8
  -> fsp-srv IPC
  -> FS sysmodule
  -> embedded prFile FAT layer
  -> PF_CHARCODE conversion table
  -> CP932 / Shift-JIS codecvt functions
  -> FAT long filename UTF-16 entries
```

Older tests showed that changing libnx conversion helpers does not recover the lost characters. That is expected: by the time libnx receives directory entries from FS, non-CP932 characters have already been converted incorrectly by FS.

The stronger reverse-engineering result is that the FS process contains CP932 conversion functions and a writable codeset function-pointer table matching the prFile `PF_CHARCODE` shape:

```text
+0x00 oem2unicode
+0x08 unicode2oem
+0x10 oem_char_width
+0x18 is_oem_mb_char
+0x20 unicode_char_width
+0x28 is_unicode_mb_char
```

For firmware 19.0.1, the analyzed dump showed CP932 conversion routines and a CP932 table matching known prFile-derived sources.

## What the simple patch does

The tested lightweight patch disables the branch into CP932 table handling in both directions:

```text
OEM2Unicode:  byte -> u16 low byte
Unicode2OEM:  u16 low byte -> byte
```

In practice this means UTF-8 path bytes are preserved if the files are created and later read by the patched Switch environment.

This is why names such as Chinese, Japanese, and Korean can appear to work after the patch when the Switch itself creates the files. The FAT LFN entries are not standard UTF-16 names for those characters; they are byte-preserving entries.

## What the simple patch cannot do

It cannot decode normal PC-created FAT long filenames.

A PC-created file named `卡带` is stored as real UTF-16 code units `U+5361 U+5E26`. The byte-preserving patch expects UTF-8 bytes widened to `u16`, such as `U+00E5 U+008D U+00A1 ...`. These are different on-disk names.

So the current statement should be:

```text
The simple byte-preserving patch cannot support standard PC-written UTF-16 FAT long filenames.
```

It should not be overstated as:

```text
Switch SFAT can never support standard UTF-16 FAT long filenames.
```

A real UTF-8 <-> UTF-16 codecvt replacement should be conceptually possible, but it is a larger patching problem.

## What a full fix would need

A complete fix would replace or redirect the CP932 codecvt layer with real Unicode conversion:

```text
application UTF-8 bytes <-> FAT LFN UTF-16 code units
```

At minimum it needs correct replacements for:

```text
oem2unicode
unicode2oem
oem_char_width
is_oem_mb_char
unicode_char_width
is_unicode_mb_char
```

The hard parts are practical rather than theoretical:

- finding stable code space for injected conversion functions;
- surviving ASLR;
- replacing the writable codeset function pointers safely;
- implementing UTF-8 validation and error behavior compatible with FS expectations;
- handling non-BMP characters if desired;
- testing directory iteration, create, open, rename, delete, and mixed PC/Switch workflows.

## Practical compatibility table

| Scenario | Stock FS | Simple byte-preserving patch | Full UTF-8/UTF-16 codecvt |
| --- | --- | --- | --- |
| ASCII paths | Works | Works | Works |
| CP932-representable Japanese paths | Often works | May preserve bytes, but representation changes | Works if implemented correctly |
| PC-created Chinese UTF-16 LFN | Fails or `_` | Still fails | Should work |
| Switch-created UTF-8 Chinese path | Fails or `_` | Works in patched Switch environment | Works and remains PC-compatible |
| Same SD card shared with PC | Broken for many CJK names | Not standard for patched-created names | Target behavior |

## Current conclusion

The root bug is very likely Nintendo's FS codecvt choice for the SD FAT path: CP932 is being used where a Unicode-aware UTF-8/UTF-16 conversion is needed.

The two-NOP patch is valuable because it proves that the bad CP932 branch is the choke point and provides a small runtime workaround for Switch-only workflows. But it is not the final compatibility fix. The final fix is replacing the prFile character-code table with a real UTF-8/UTF-16 implementation.
