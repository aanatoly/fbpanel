# Installation:

## User install
Build and install it
```
./configure --prefix=$HOME/.local
make
make install
```

Then add `~/.local/bin/` to your `PATH`. Add this line to `~/.bashrc`
```
export PATH=$HOME/.local/bin:$PATH
```

## System Install
Build and install it
```
./configure
make
su -c "make install"
```

# Dependencies
Deps:
 * core - gtk2 2.17 or higher
 * plugin `foo` - bar 1.0
