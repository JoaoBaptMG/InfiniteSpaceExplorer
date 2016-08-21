#!/bin/sh

# Make the structure directories
mkdir out
mkdir out/Rescaled1x out/Rescaled2x out/Rescaled3x
mkdir out/UI.Rescaled1x out/UI.Rescaled2x out/UI.Rescaled3x

# Check the main files
for f in $(ls in);
do
    sips --resampleHeight $((`./height in/$f`/4)) in/$f --out out/Rescaled1x/$f
    sips --resampleHeight $((`./height in/$f`/2)) in/$f --out out/Rescaled2x/$f
    sips --resampleHeight $((`./height in/$f`*3/4)) in/$f --out out/Rescaled3x/$f
    img1x="$img1x\nout/Rescaled1x/$f"
    img2x="$img2x\nout/Rescaled2x/$f"
    img3x="$img3x\nout/Rescaled3x/$f"
    img4x="$img4x\nin/$f"
done

for f in $(ls ui.in);
do
    sips --resampleHeight $((`./height ui.in/$f`/4)) ui.in/$f --out out/UI.Rescaled1x/$f
    sips --resampleHeight $((`./height ui.in/$f`/2)) ui.in/$f --out out/UI.Rescaled2x/$f
    sips --resampleHeight $((`./height ui.in/$f`*3/4)) ui.in/$f --out out/UI.Rescaled3x/$f
    ui1x="$ui1x\nout/UI.Rescaled1x/$f"
    ui2x="$ui2x\nout/UI.Rescaled2x/$f"
    ui3x="$ui3x\nout/UI.Rescaled3x/$f"
    ui4x="$ui4x\nui.in/$f"
done

# Pack the textures
echo "$img1x" | ./texpack --trim --allow-rotate --POT --max-size 1024x1024 --padding 4 --output assets1x/GameAssets
echo "$img2x" | ./texpack --trim --allow-rotate --POT --max-size 2048x2048 --padding 4 --output assets2x/GameAssets
echo "$img3x" | ./texpack --trim --allow-rotate --POT --max-size 4096x4096 --padding 4 --output assets3x/GameAssets
echo "$img4x" | ./texpack --trim --allow-rotate --POT --max-size 4096x4096 --padding 4 --output assets4x/GameAssets

echo "$ui1x" | ./texpack --trim --allow-rotate --POT --max-size 1024x1024 --padding 4 --output assets1x/UIAssets
echo "$ui2x" | ./texpack --trim --allow-rotate --POT --max-size 2048x2048 --padding 4 --output assets2x/UIAssets
echo "$ui3x" | ./texpack --trim --allow-rotate --POT --max-size 4096x4096 --padding 4 --output assets3x/UIAssets
echo "$ui4x" | ./texpack --trim --allow-rotate --POT --max-size 4096x4096 --padding 4 --output assets4x/UIAssets

plutil -insert metadata.textureFileName -string 'GameAssets.png' -- assets1x/GameAssets.plist
plutil -insert metadata.textureFileName -string 'GameAssets.png' -- assets2x/GameAssets.plist
plutil -insert metadata.textureFileName -string 'GameAssets.png' -- assets3x/GameAssets.plist
plutil -insert metadata.textureFileName -string 'GameAssets.png' -- assets4x/GameAssets.plist

plutil -insert metadata.textureFileName -string 'UIAssets.png' -- assets1x/UIAssets.plist
plutil -insert metadata.textureFileName -string 'UIAssets.png' -- assets2x/UIAssets.plist
plutil -insert metadata.textureFileName -string 'UIAssets.png' -- assets3x/UIAssets.plist
plutil -insert metadata.textureFileName -string 'UIAssets.png' -- assets4x/UIAssets.plist

# Remove the directory
rm -rf out