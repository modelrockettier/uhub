# uHub

[![Cirrus status][cirrus-img]](https://cirrus-ci.com/github/modelrockettier/uhub)
[![Travis status][travis-img]](https://travis-ci.com/modelrockettier/uhub)
[![Docker Hub status][docker-img]](https://hub.docker.com/r/modelrockettier/uhub)
[![Quay container status][quay-img]](https://quay.io/repository/modelrockettier/uhub)
[![Latest release][release-img]](https://github.com/modelrockettier/uhub/releases)
[![Commits since latest release][commits-img]](https://github.com/modelrockettier/uhub/commits/master)
[![License: GPL v3][license-img]](https://www.gnu.org/licenses/gpl-3.0)

[cirrus-img]:  https://img.shields.io/cirrus/github/modelrockettier/uhub/master?label=cirrus%20build&cacheSeconds=300 "Cirrus CI build status"
[travis-img]:  https://img.shields.io/travis/com/modelrockettier/uhub/master?label=travis%20build&cacheSeconds=300 "Travis CI build status"
[docker-img]:  https://img.shields.io/docker/cloud/build/modelrockettier/uhub?label=docker%20hub&cacheSeconds=300 "Docker Hub build status"
[quay-img]:    https://quay.io/repository/modelrockettier/uhub/status "Quay container build status"
[release-img]: https://img.shields.io/github/v/release/modelrockettier/uhub?cacheSeconds=1800 "Latest GitHub release"
[commits-img]: https://img.shields.io/github/commits-since/modelrockettier/uhub/latest/master?cacheSeconds=1800 "GitHub commits since latest release"
[license-img]: https://img.shields.io/badge/License-GPLv3-blue.svg?label=license&cacheSeconds=3600 "License"


Uhub is a high performance peer-to-peer hub for the ADC network.

Its low memory footprint allows it to handle several thousand users on
high-end servers, or a small private hub on embedded hardware.

Uhub uses the ADC protocol, and is compatible with DC++ and other ADC clients.

## Key features

 - High performance and low memory usage
 - IPv4 and IPv6 support
 - SSL/TLS support (optional)
 - Easy configuration
 - Plugin support

## Download

You can find zip / tar.gz files of different uHub versions on
https://github.com/janvidar/uhub/releases

You can also use git to use the last up-to-date version:
```shell
git clone https://github.com/modelrockettier/uhub.git
```

### Ubuntu and Debian packages

Tehnick has provided an up to date uHub PPA for Debian and Ubuntu based
distributions: https://tehnick.github.io/uhub/

## Docker

See the [doc/docker.md](doc/docker.md) document
([permalink](https://github.com/modelrockettier/uhub/blob/master/doc/docker.md))
for more information and examples.

```shell
docker run -v /uhub:/conf -p 1511:1511 modelrockettier/uhub
```

## Documentation

### Compile

See the [doc/compile.md](doc/compile.md) document
([permalink](https://github.com/modelrockettier/uhub/blob/master/doc/compile.md))

### Getting started

See the [doc/getstarted.md](doc/getstarted.md) document
([permalink](https://github.com/modelrockettier/uhub/blob/master/doc/getstarted.md))

### Use TLS/SSL

See the [doc/tls-documentation.md](doc/tls-documentation.md) document
([permalink](https://github.com/modelrockettier/uhub/blob/master/doc/tls-documentation.md))

## Compatible clients

For a list of compatible ADC clients, see:
<https://en.wikipedia.org/wiki/Comparison_of_ADC_software#Client_software>

## License

Uhub is free and open source software, licensed under the
GNU General Public License version 3 or later.

See [COPYING](COPYING) document
([permalink](https://github.com/janvidar/uhub/blob/master/COPYING))
or <https://www.gnu.org/licenses/gpl-3.0>
