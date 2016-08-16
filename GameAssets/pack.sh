#!/bin/sh

# Make the structure directories
mkdir out
mkdir out/Rescaled1x out/Rescaled2x out/Rescaled3x
mkdir out/UI.Rescaled1x out/UI.Rescaled2x out/UI.Rescaled3x

# Check the main files
for f in $(ls in);
do
    echo in/$f
    python resize.py -s 0.25 in/$f out/Rescaled1x/$f &
    python resize.py -s 0.50 in/$f out/Rescaled2x/$f &
    python resize.py -s 0.75 in/$f out/Rescaled3x/$f
done

for f in $(ls ui.in);
do
    echo ui.in/$f
    python resize.py -s 0.25 ui.in/$f out/UI.Rescaled1x/$f &
    python resize.py -s 0.50 ui.in/$f out/UI.Rescaled2x/$f &
    python resize.py -s 0.75 ui.in/$f out/UI.Rescaled3x/$f
done

# Pack the textures
java -jar TexturePacker.jar out/Rescaled1x --trim --rotation --force-squared --max-size=1024 --shape-padding=4 --texture="assets1x/GameAssets.png" --data="assets1x/GameAssets.plist" &
java -jar TexturePacker.jar out/Rescaled2x --trim --rotation --force-squared --max-size=2048 --shape-padding=4 --texture="assets2x/GameAssets.png" --data="assets2x/GameAssets.plist" &
java -jar TexturePacker.jar out/Rescaled3x --trim --rotation --force-squared --max-size=4096 --shape-padding=4 --texture="assets3x/GameAssets.png" --data="assets3x/GameAssets.plist" &
java -jar TexturePacker.jar in --trim --rotation --force-squared --max-size=4096 --shape-padding=4 --texture="assets4x/GameAssets.png" --data="assets4x/GameAssets.plist"

java -jar TexturePacker.jar out/UI.Rescaled1x --trim --rotation --force-squared --max-size=1024 --shape-padding=4 --texture="assets1x/UIAssets.png" --data="assets1x/UIAssets.plist" &
java -jar TexturePacker.jar out/UI.Rescaled2x --trim --rotation --force-squared --max-size=2048 --shape-padding=4 --texture="assets2x/UIAssets.png" --data="assets2x/UIAssets.plist" &
java -jar TexturePacker.jar out/UI.Rescaled3x --trim --rotation --force-squared --max-size=4096 --shape-padding=4 --texture="assets3x/UIAssets.png" --data="assets3x/UIAssets.plist" &
java -jar TexturePacker.jar ui.in --trim --rotation --force-squared --max-size=4096 --shape-padding=4 --texture="assets4x/UIAssets.png" --data="assets4x/UIAssets.plist"

echo Done!

read

# Remove the directory
rm -rf out