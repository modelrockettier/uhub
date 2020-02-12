# How to compile:

## Prerequisites

Before you try to compile µHub, please make sure the following prerequisites are met.
  * GNU make
  * gcc > 3.0 or clang (or MinGW on Windows)
  * Perl 5
  * openssl >= 1.1 (or use `make USE_SSL=NO`)
  * sqlite > 3.x

For Ubuntu / Debian:
```
sudo apt-get install cmake make gcc git libsqlite3-dev libssl-dev
```

## Linux, Mac OSX, FreeBSD, NetBSD and OpenBSD
```
cmake .
make
sudo make install
```

If you have an old gcc compiler, try disabling pre-compiled headers like this:
```
gmake USE_PCH=NO
```

### Default install directories:

| What                | Where                      |
| ---                 | ---                        |
| Binaries            | /usr/local/bin/            |
| Configuration files | /etc/uhub/                 |
| Plugins             | /usr/local/lib/uhub/       |
| Manual pages        | /usr/local/share/man/man1/ |
