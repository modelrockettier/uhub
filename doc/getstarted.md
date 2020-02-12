# Getting started guide

## Compile it at first

See the [compile.md](compile.md) file

## Create configuration files

If no configuration files are created, uhub will use the default parameters, so you can skip this step if you are in a hurry to see it run.

As root, or use sudo.
```
mkdir /etc/uhub
cp doc/uhub.conf /etc/uhub
cp doc/users.conf /etc/uhub
echo "welcome to uhub" > /etc/uhub/motd.txt
```

You can edit the default configuration file before starting uhub.
As root edit /etc/uhub/uhub.conf:
```
  hub_name=My Public Hub
  hub_description=Yet another ADC hub

  server_port=1511
  server_bind_addr=any
  max_users=150
```

## Start uhub

Start the hub in the foreground for the first time. Shut it down, by pressing CTRL+C.
```
% uhub
Thu, 05 Feb 2009 00:48:04 +0000 INFO: Starting server, listening on :::1511...
```
Connect to the hub using an ADC client, use the address adc://localhost:1511, or replace localhost with the correct hostname or IP address.

**NOTE**: It is important to use the "adc://" prefix, and the port number.

## Kill / Stop uhub

If you modify the configuration files in /etc/uhub you will have to notify uhub by sending a HUP signal.
```
ps aux | grep uhub
kill -HUP <pid of uhub>
```

Or, for the lazy people
```
killall -HUP uhub
```

## Start uhub as daemon (or in background mode)

In order to run uhub as a daemon, start it with the -f switch which will make it fork into the background.

In addition, use the -l to specify a log file instead of stdout.

To run uhub as a specific user, use the -u and -g switches.

Example:
```
uhub -f -l mylog.txt -u nobody -g nogroup
```

## Having more than 1024 users on uhub

If you are planning to more than 1024 users on your hub, you must increase the max number of file descriptors allowed.

This limit needs to be higher than the configured max_users in uhub.conf.

In Linux can add the following lines to /etc/security/limits.conf (allows for ~4000 users)
```
soft nofile 4096
hard nofile 4096
```

Or, you can use (as root):
```
ulimit -n 4096
```

## Interact with uhub through your hub chat (only for operator/admin):

You can interact with uhub in your hub main chat using the `!` prefix, followed by a command:

Example, to display help and the command you can use:
```
!help
```
Your mileage may vary -- Good luck!