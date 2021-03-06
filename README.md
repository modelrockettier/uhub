# uHub

[![Cirrus][cirrus-img]](https://cirrus-ci.com/github/modelrockettier/uhub)
[![Travis][travis-img]](https://travis-ci.org/modelrockettier/uhub)
[![Docker Hub][docker-img]](https://hub.docker.com/r/modelrockettier/uhub)
[![Quay.io][quay-img]](https://quay.io/repository/modelrockettier/uhub)
[![Coverity][coverity-img]](https://scan.coverity.com/projects/modelrockettier-uhub)
[![SonarCloud][sonarcloud-img]](https://sonarcloud.io/dashboard?id=modelrockettier_uhub)
[![Releases][release-img]](https://github.com/modelrockettier/uhub/releases)
[![GPLv3+][license-img]](https://www.gnu.org/licenses/gpl-3.0)

[cirrus-img]:     https://api.cirrus-ci.com/github/modelrockettier/uhub.svg?branch=master "Cirrus CI build status"
[travis-img]:     https://api.travis-ci.com/modelrockettier/uhub.svg?branch=master "Travis CI build status"
[docker-img]:     https://img.shields.io/docker/cloud/build/modelrockettier/uhub?label=docker&cacheSeconds=300 "Docker Hub build status"
[quay-img]:       https://quay.io/repository/modelrockettier/uhub/status "Quay.io build status"
[coverity-img]:   https://scan.coverity.com/projects/20832/badge.svg "Coverity status"
[sonarcloud-img]: https://sonarcloud.io/api/project_badges/measure?project=modelrockettier_uhub&metric=alert_status "SonarCloud Status"
[release-img]:    https://img.shields.io/github/v/release/modelrockettier/uhub?cacheSeconds=3600 "Latest GitHub release"
[license-img]:    https://img.shields.io/badge/License-GPLv3-blue.svg?label=license&cacheSeconds=3600 "License"


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
https://github.com/modelrockettier/uhub/releases

You can also use git to use the last up-to-date version:
```shell
git clone https://github.com/modelrockettier/uhub.git
```

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

#### Create a .deb package

See the [doc/deb-package.md](doc/deb-package.md) document
([permalink](https://github.com/modelrockettier/uhub/blob/master/doc/deb-package.md))
for info on creating an installable package for Debian, Ubuntu, etc.

#### Create an .rpm package

See the [doc/rpm-package.md](doc/rpm-package.md) document
([permalink](https://github.com/modelrockettier/uhub/blob/master/doc/rpm-package.md))
for info on creating an installable package for Fedora, CentOS, etc.

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
