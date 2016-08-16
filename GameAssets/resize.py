#!/user/bin/python

import sys
import PIL
from PIL import Image

mode = sys.argv[1]
size = sys.argv[2]
input = sys.argv[3]
output = sys.argv[4]

img = Image.open(input)

if mode.upper() == "-S":
    img.thumbnail(tuple(x*float(size) for x in img.size))
elif mode.upper() == "-R":
    img.thumbnail(tuple(int(size) for x in img.size))
else:
    print("Unexpected mode!")

img.save(output)
