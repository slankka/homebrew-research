# Homebrew research
About the Horizon OS handling unicode characters.

## the background
I'm a RetroArch user, one day I copy a brunch of files and folders to Switch SdCard, many directories named as none-ascii characters are not displayed correctly.

The part of DBCS(double byte character set) are replaced by `_` underscore.

If the directory of file name is consists of full DBCS characters, the whole directory is not displayed.

If the folders and files are too many, it's difficult to manage. So I want to know if there is any way to handle it.

## The TOC

[libnx and unicode](unicode-stories.md)
- the bug detail
- my research about how retroarch failed to scan unicode paths
- Begin journey: How retroarch calling to Libnx
- Deep Search: How libnx calling to fs-srv
- HIPC  (Horizon Inter-Process Communication)
- CMIF (Command Interface Framework)
- The suspicious part of Directory iteration
- What I have tried?

[directory-struct-of-libnx](directory-struct-of-libnx.md)
- DirectoryEntry (ns)
- DirectoryEntry （3Ds）
    - Attributes （3DS）



[hipc-cmif](hipc-cmif.md)
- key source of hipc service wrapper

[emummc-filesystem](emummc-filesystem.md)
- How does EmuMMC connect to FatFS?
- How does FatFs work? what role does it play?
- The DiskIO layer (interface)
- how does emummc plug into FatFs's disk_read or disk_write?
- the DMA address binding 
- How does the EmuMMC self-infesting work?
- How does Emummc treat the different paths?


# Some conclusion

1. there is a few of libs or homebrews treat none-ascii chars as '_'.
  - sush as fatfs (without unicode).
  - I can see many homebrews turn off fatfs unicode support. 

2. The AtmosphereNX has two ways to access SD card: File-Based Partition and Raw Partition. 
  - File-Based partition, the AI mentioned that we can implement another Fs and driver to mount a file image. 
    There are tools like `NxNandManager`, we can use it to make rom images and mount it. Maybe further retroarch customizing required.
  - Raw partition, some user says the lakka build of Libretro can work with it.

3. The nintendo is very professional at game console, how could they make mistake on file system service? we can see the switchbrew posted DirectoryEntry class fields, the 3ds using UTF16 names and 8.3 SFN, what employee can build a new system without checking the previous system documents!!

4. There is a project [ftpsrv](https://github.com/ITotalJustice/ftpsrv) , the autor recently solves a game content unicode character problem. It is a special case that the game info stored in NACP file. The unicode characters is read and filtered before by replace the none-ascii character with `_`, he add `g_skip_ascii_convert` to skip the conversion, see [ftpsrv-issue 11](https://github.com/ITotalJustice/ftpsrv/issues/11). So the game info is displayed correctly. The key point is the game info is stored in a database file.



