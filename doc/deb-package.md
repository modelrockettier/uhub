# How to create a debian package

For Debian, Ubuntu, Mint, etc. (a .deb file).

## Install Prerequisites

```shell
sudo apt-get install cmake dpkg-dev gcc git libsqlite3-dev libssl-dev make
```

## Build the debian package
```shell
dpkg-buildpackage -b -us -uc
```

This will create a .deb file in the parent directory (e.g.
`../uhub_0.5.1-1_amd64.deb`), so to install it you can just run:
```shell
sudo dpkg -i ../uhub_0.5.1-1_amd64.deb
```

If you try to install the .deb package on another machine and get an error
about the package depending on something that isn't installed, you can fix it
by installing the missing dependencies with:
```shell
sudo apt-get -f install
```

### Default install directories:

| What                | Where                |
| ---                 | ---                  |
| Binaries            | /usr/bin/            |
| Configuration files | /etc/uhub/           |
| Plugins             | /usr/lib/uhub/       |
| Manual pages        | /usr/share/man/man1/ |
