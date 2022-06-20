# Laser-Display
Program to display images and movies with a show laser device using openCV

## Install
### Requirements
The SDL library must be present. On a mac, the SDL library can be installed with the following command:
```
sudo port install libsdl2 libsdl2_gfx libsdl2_image libsdl2_mixer libsdl2_ttf libftd2xx
```
### Cloning the source code
```
git clone --recurse-submodules https://github.com/MKesenheimer/Laser-Display.git
```

## Compiling
make libs
make -j4 && ./laser-display -i videos/doom.mp4 -f 10 -x 900 -y 600 -c 150,50,150,100

or

make all

## Unittests
make -j4 gtest && ./gtest