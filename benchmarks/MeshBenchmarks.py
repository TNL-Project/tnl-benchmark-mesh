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
        out = open("{}/{}.t{}.cpp".format(dirname, basename, i), "w")
        out.write("#include \"../{}.h\"\n".format(basename))
        out.write(line.strip().replace("extern ", "", 1))
        out.close()
        i += 1
