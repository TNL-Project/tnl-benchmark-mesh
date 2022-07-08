#! /usr/bin/env python3

import os.path
import shutil

basename = os.path.splitext(os.path.basename(__file__))[0]
dirname = "{}.templates".format(basename)
src = basename + ".h"

if os.path.isdir(dirname):
    shutil.rmtree(dirname)
os.mkdir(dirname)

i = 0
for line in open(src).readlines():
    if line.strip().startswith("extern template"):
        for ext in ["cpp", "cu"]:
            output_name = "{}/{}.t{}.{}".format(dirname, basename, i, ext)
            out = open(output_name, "w")
            out.write("#include \"../{}.h\"\n".format(basename))
            out.write(line.strip().replace("extern ", "", 1))
            out.close()
            # copy access and modification times from src to output_name
            stat = os.stat(src)
            os.utime(output_name, times=(stat.st_atime, stat.st_mtime))
        i += 1
