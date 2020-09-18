# How to create an RPM package

For RedHat, CentOS, Fedora, etc.

## Install Prerequisites

```shell
sudo yum install cmake gcc git make openssl-devel rpm-build sqlite-devel
```

## Build the rpm package
```shell
cmake . -DCMAKE_BUILD_TYPE=Release -DHARDENING=ON \
    -DCMAKE_INSTALL_PREFIX=/usr -DPLUGIN_DIR=/usr/lib/uhub
cpack -G RPM
```

This will create an .rpm file in the current directory (e.g.
`uhub-0.5.1-Linux.rpm`), so to install it you can just run:
```shell
sudo yum install uhub-0.5.1-Linux.rpm
```

### Default install directories:

| What                | Where                |
| ---                 | ---                  |
| Binaries            | /usr/bin/            |
| Configuration files | /etc/uhub/           |
| Plugins             | /usr/lib/uhub/       |
| Manual pages        | /usr/share/man/man1/ |
