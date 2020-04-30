Local docker-based testing, so you can check for Linux CI failures
before pushing a build to the remote CI servers.

By default, you can just run `make` to test the main Dockerfile along with:
* ubuntu:latest
* ubuntu:xenial
* centos:8
* centos:7

The `minimal`, `full`, `deb`, and `rpm` CONFIG settings will be tested
(where applicable).  It does **NOT** test Windows, Mac, FreeBSD, other
CPU architectures, or the Clang compiler.

You can also specify specific any other alpine, centos, debian, or ubuntu
docker tags to build against (just replace the `:` with `-`).

E.g.
* Test just the main Dockerfile:
```
make main
```

* Test CentOS 7 and 8
```
make centos-7 centos-8
```

* Test Ubuntu Xenial, Bionic, and Focal (16.04, 18.04, and 20.04)
```
make ubuntu-xenial ubuntu-bionic ubuntu-focal
```

* Test the latest Alpine and Debian Stretch and Buster (in parallel)
```
make -j3 alpine-latest debian-stretch debian-buster
```
