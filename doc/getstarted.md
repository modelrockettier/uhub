# Getting started guide

[Docker](docker.md) is the quickest and easiest way to set up a uhub server
([instructions permalink][1]).

This guide describes how to set up a uhub server *without* using docker.

## Compile it at first

If you're on a Debian-based distribution (such as Ubuntu), see the
[deb-package.md](deb-package.md) document ([permalink][2])
for instructions on creating an installable .deb package.

If you're on an RPM-based distribution (such as CentOS or Fedora), see the
[rpm-package.md](rpm-package.md) document ([permalink][3])
for instructions on creating an installable .rpm package.

If you're on a different platform, or don't want to create an installable
package, see the [compile.md](compile.md) document ([permalink][4]) for
instructions on how to compile uhub.

## Create configuration files

If no configuration files are created, uhub will use the default parameters,
so you can skip this step if you are in a hurry to see it run.

As root, or use sudo.
```shell
mkdir /etc/uhub
cp doc/uhub.conf /etc/uhub
cp doc/users.conf /etc/uhub
echo "welcome to uhub" > /etc/uhub/motd.txt
```

You can edit the default configuration file before starting uhub.
As root edit `/etc/uhub/uhub.conf`:
```
hub_name=My Public Hub
hub_description=Yet another ADC hub

server_port=1511
server_bind_addr=any
max_users=150
```

## Start uhub

Start the hub in the foreground for the first time. Shut it down, by pressing
CTRL+C.
```
% uhub
Thu, 05 Feb 2009 00:48:04 +0000 INFO: Starting server, listening on :::1511...
```
Connect to the hub using an ADC client, use the address `adc://localhost:1511`,
or replace localhost with the correct hostname or IP address.

**NOTE**: It is important to use the `adc://` prefix, and the port number.

## Kill / Stop uhub

If you modify the configuration files in `/etc/uhub` you will have to notify
uhub by sending a HUP signal.
```shell
ps aux | grep uhub
kill -HUP <pid of uhub>
```

Or, for the lazy people
```shell
killall -HUP uhub
```

## Start uhub as daemon (or in background mode)

In order to run uhub as a daemon, start it with the `-f` switch which will make
it fork into the background.

In addition, use the `-l` to specify a log file instead of stdout.

To run uhub as a specific user, use the `-u` and `-g` switches.

Example:
```shell
uhub -f -l mylog.txt -u nobody -g nogroup
```

## Having more than 1024 users on uhub

If you are planning to more than 1024 users on your hub, you must increase the
max number of file descriptors allowed.

This limit needs to be higher than the configured max_users in uhub.conf.

In Linux can add the following lines to `/etc/security/limits.conf`
(allows for ~4000 users)
```
soft nofile 4096
hard nofile 4096
```

Or, you can use (as root):
```shell
ulimit -n 4096
```

## Interact with uhub through your hub chat:

You can interact with uhub in your hub main chat using the `!` prefix,
followed by a command:

E.g. to display help and the commands you can use:
```
!help
```

For more information about a specific command (e.g. `!myip`), you can use:
```
!help myip
```

Your mileage may vary -- Good luck!

[1]: https://github.com/modelrockettier/uhub/blob/master/doc/docker.md
[2]: https://github.com/modelrockettier/uhub/blob/master/doc/deb-package.md
[3]: https://github.com/modelrockettier/uhub/blob/master/doc/rpm-package.md
[4]: https://github.com/modelrockettier/uhub/blob/master/doc/compile.md
