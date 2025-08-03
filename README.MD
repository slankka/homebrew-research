# Homebrew research
About the Horizon OS handling unicode characters.

## the background
I'm a RetroArch user, I used to run RetroArch on PSV, and it works very well.

Now when I copy my retro game roms (a brunch of files and folders) to Switch, many of roms named as none-ascii characters, it's not easy to manage.

So I want to know if there is any way to handle it.

## The TOC

[libnx and unicode](unicode-stories.md)

[directory-struct-of-libnx](directory-struct-of-libnx.md)

[hipc-cmid](hipc-cmid.md)


# Some conclusion

1. there are many libs or homebrews actuals treats none-ascii chars as '_', sush as fatfs. (without unicode)
2. The AtmosphereNX has two ways to access SD card by Fatfs: File-Based Partition and Raw Partition. I think both way may work except accessing HOS kernel fs service. 
3. File-Based partition, the AI mentioned that we can implement another Fs and driver to mount a file image. 
    There are tools like `NxNandManager`, we can use it to make rom images and mount it.
4. Raw partition, some user says the lakka build of Libretro can work with it.