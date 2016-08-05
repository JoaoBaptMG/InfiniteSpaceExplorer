#!/user/bin/python

import sys
import PIL
from PIL import Image

size = sys.argv[1]
input = sys.argv[2]
output = sys.argv[3]

img = Image.open(input)
img.thumbnail(tuple(x*float(size) for x in img.size))
img.save(output)
