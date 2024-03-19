import os
os.system('firmware_version.bat')

file = open("firmware_version.txt", "rt")
out_line = ''
for line in file:
    l = line.strip()
    if len(l):
        out_line += l + ' '
out_line = '#define SW_VERSION "' + out_line.strip() + '"'
file.close()

file = open("main/firmware_version.h", "wt")
file.write(out_line)
file.close()

print(out_line)
