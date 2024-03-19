::git config --get remote.origin.url > firmware_version.txt
git branch --show-current > firmware_version.txt
git log -1 --format="%%H" >> firmware_version.txt

rmdir /Q /S ".pio\\libdeps\\esp32-s3-devkitc-1_DIY1\\esp-idf-speex\\speex\\ti"
rmdir /Q /S ".pio\\libdeps\\esp32dev\\esp-idf-speex\\speex\\ti"

del /Q /S   ".pio\\libdeps\\esp32-s3-devkitc-1_DIY1\\esp-idf-speex\\speex\\src\\skeleton.c"
del /Q /S   ".pio\\libdeps\\esp32dev\\esp-idf-speex\\speex\\src\\skeleton.c"

del /Q /S   ".pio\\libdeps\\esp32-s3-devkitc-1_DIY1\\esp-idf-speex\\speex\\src\\speexdec.c"
del /Q /S   ".pio\\libdeps\\esp32dev\\esp-idf-speex\\speex\\src\\speexdec.c"

del /Q /S   ".pio\\libdeps\\esp32-s3-devkitc-1_DIY1\\esp-idf-speex\\speex\\src\\speexenc.c"
del /Q /S   ".pio\\libdeps\\esp32dev\\esp-idf-speex\\speex\\src\\speexenc.c"

::pause
