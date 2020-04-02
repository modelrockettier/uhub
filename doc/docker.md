# Docker

## Registries

You can find uhub docker images in the following container registries:

* [Docker Hub](https://hub.docker.com/r/modelrockettier/uhub)
* [Quay.io](https://quay.io/repository/modelrockettier/uhub)

## Quick Start

```shell
docker run --name uhub -v /srv/uhub:/conf -p 1511:1511 modelrockettier/uhub
```

## First-Time Setup

The first time you run the uhub container, it will automatically create an
admin user and copy the default configuration to your `/conf` volume.
The admin user name and password will be printed in the container logs
(`docker logs uhub`) and saved to `/conf/admin.txt`.

- If the `UHUB_ADMIN` environment variable is not specified, it will be `admin`.
- If the `UHUB_PASSWD` environment variable is not specified or is empty, it
  will be randomly generated.

After the first run, you can change the admin password with the
[uhub-passwd](#the-uhub-passwd-command) command.

## Typical Usage

Create a dedicated unprivileged user for uhub.

```shell
useradd -r -s /usr/sbin/nologin -c uHub -d /srv/uhub uhub
```

Typical uhub server:
 - Uses the hosts' timezone (if you're on a system that has `/etc/localtime`
   such as Debian or Ubuntu)
 - Will restart itself automatically if the server goes down for any reason
   (except `docker stop`)
 - Uses its own user and drops unnecessary root/admin privileges
 - Prints more verbose output (`UHUBOPT`)
 - Sets the admin username and password (optional, only used for the first run)

```shell
docker run --detach \
    --restart unless-stopped \
    --name uhub \
    --env UHUBOPT=-v \
    --env UHUB_ADMIN=myadmin \
    --env UHUB_PASS=mypassword \
    --user uhub \
    --cap-drop ALL \
    --volume /etc/localtime:/etc/localtime:ro \
    --volume /srv/uhub:/conf:Z \
    --publish 1511:1511 \
    --memory 512M \
    modelrockettier/uhub
```

Simple server using the quay.io container image instead of docker hub

```shell
docker run --name uhub -v /srv/uhub:/conf -p 1511:1511 quay.io/modelrockettier/uhub
```

## Environment Variables

* `UHUBOPT` - Additional command-line arguments to pass to uhub.
* `UHUB_ADMIN` - The name of the admin account to create the first time uhub is run.
* `UHUB_PASS` - The password for the admin account to create the first time uhub is run.

## Volumes

* `/conf` - The uhub configuration and log directory

## Useful File Locations

* `/libs` - Where the uhub plugins are stored

* `/app/bin` - Where the `uhub` and `uhub-passwd` binaries are located

* `/app/man` - Where the uhub manual pages are located

## The uhub-passwd Command

The `uhub-passwd` command is used in conjunction with the `mod_auth_sqlite`
plugin (enabled by default in `/conf/plugins.conf`).

After the initial setup, you can change the admin user on a **running**
uhub container with:

```shell
docker exec "container" uhub-passwd /conf/users.db pass "admin" "new-password"
```

In addition to changing password, the `uhub-passwd` utility also supports
adding, removing, promoting/demoting, and listing users.

Show help text

```shell
docker exec "container" uhub-passwd /conf/users.db help
```

List all users

```shell
docker exec "container" uhub-passwd /conf/users.db list
```

Add a new user
```shell
docker exec "container" uhub-passwd /conf/users.db add "bob" "password"
```
