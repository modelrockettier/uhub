# How to compile:

## Prerequisites

Before you try to compile uHub, please make sure the following prerequisites are met.
 * a C compiler (gcc > 3.0, clang, MinGW or MS Visual Studio)
 * GNU make
 * CMake
 * Perl 5
 * openssl >= 1.1 (or use `make USE_SSL=NO`)
 * sqlite >= 3.x
 * pkg-config (if `USE_SYSTEMD=ON`)
 * systemd development headers (if `USE_SYSTEMD=ON`)

For Ubuntu / Debian:
```shell
sudo apt-get install cmake make gcc git libsqlite3-dev libssl-dev
```

## Unix-like systems (Linux, Mac OSX, BSD, etc)
```shell
cmake .
make
sudo make install
```

If you have an old gcc compiler, try disabling pre-compiled headers like this:
```shell
make USE_PCH=NO
```

### Default install directories:

| What                | Where                      |
| ---                 | ---                        |
| Binaries            | /usr/local/bin/            |
| Configuration files | /etc/uhub/                 |
| Plugins             | /usr/local/lib/uhub/       |
| Manual pages        | /usr/local/share/man/man1/ |
