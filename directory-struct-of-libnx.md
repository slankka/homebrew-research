### DirectoryEntry (ns)

This is "nn::fs::DirectoryEntry".

|Offset|Size|Description|
|-|-|-|
|0x0|0x301|Path|
|0x301|0x1|File attributes (bit 0 = is directory; bit 1 = archive bit)|
|0x302|0x2|Padding? (UTF16?)|
|0x304|0x1|[#DirectoryEntryType](https://switchbrew.org/wiki/Filesystem_services#DirectoryEntryType)|
|0x305|0x3|Padding?|
|0x308|0x8|Filesize, 0 for directories.|

**Comment:**
Compared with 3D(S) below: I think the padding is not padding:
(**isHidden??, isArchive??, is ReadOnly??, 4bytes**)

### Attributes （3DS）

|Offset|Size|Description|
|-|-|-|
|0x0|0x1|Is Directory|
|0x1|0x1|Is Hidden|
|0x2|0x1|Is Archive|
|0x3|0x1|Is Read-Only|


### DirectoryEntry （3Ds）

|Offset|Size|Description|
|-|-|-|
|0x0|0x20C|UTF-16 Entry Name|
|0x20C|0xA|8.3 short filename name|
|0x216|0x4|8.3 short filename extension|
|0x21A|0x1|Always 1|
|0x21B|0x1|Reserved|
|0x21C|0x4|[Attributes](https://www.3dbrew.org/wiki/Filesystem_services#Attributes) (4 bytes)|
|0x220|0x8|Entry Size|


[https://switchbrew.github.io/libnx/structFsFileSystemAttribute.html](https://switchbrew.github.io/libnx/structFsFileSystemAttribute.html)
