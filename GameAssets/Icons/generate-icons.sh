#!/bin/sh

python ../resize.py -r 29 IconBase.png Icon-29.png
python ../resize.py -r 58 IconBase.png Icon-58.png
python ../resize.py -r 87 IconBase.png Icon-87.png
python ../resize.py -r 40 IconBase.png Icon-40.png
python ../resize.py -r 80 IconBase.png Icon-80.png
python ../resize.py -r 120 IconBase.png Icon-120.png
python ../resize.py -r 180 IconBase.png Icon-180.png
python ../resize.py -r 76 IconBase.png Icon-76.png
python ../resize.py -r 152 IconBase.png Icon-152.png

python ../resize.py -r 36 IconBase.png icon-ldpi.png
python ../resize.py -r 48 IconBase.png icon-mdpi.png
python ../resize.py -r 72 IconBase.png icon-hdpi.png
python ../resize.py -r 96 IconBase.png icon-xhdpi.png
python ../resize.py -r 144 IconBase.png icon-xxhdpi.png
python ../resize.py -r 192 IconBase.png icon-xxxhdpi.png

cp icon-ldpi.png ../../proj.android/res/drawable-ldpi/icon.png
cp icon-mdpi.png ../../proj.android/res/drawable-mdpi/icon.png
cp icon-hdpi.png ../../proj.android/res/drawable-hdpi/icon.png
cp icon-xhdpi.png ../../proj.android/res/drawable-xhdpi/icon.png
cp icon-xxhdpi.png ../../proj.android/res/drawable-xxhdpi/icon.png
cp icon-xxxhdpi.png ../../proj.android/res/drawable-xxxhdpi/icon.png