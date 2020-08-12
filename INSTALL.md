# Installation:


## Requirements

You need recent GTK-2.0 (development of gtk-2 stopped a while ago, so in most cases you'll have correct one), at least 2.17.
Also cmake-3.5.2+ required and glib2, the most elder version tested is 2.4.46, but it looks like earlier ones might work too.


## System Install

Build and install it
```sh
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=RELWITHDEBINFO ..
make -j$(nproc)
su -c "make install"
```
