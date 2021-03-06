# Setting up a TLS/SSL hub

## About certificates

Before you can setup an TLS protected hub, you must create an TLS certificate
for the hub.

NOTE: uhub must be compiled with `SSL_SUPPORT` enabled in order for this to
work (the default).

## Creating certificates

If you already have certificates ready for uHub, skip down to
[Configuring uhub](#configuring-uhub) below.

Create either a [self-signed certificate](#creating-a-self-signed-certificate),
or a [certificate signed by a CA](#creating-a-certificate-with-a-ca)
(Certificate Authority).

### Creating a self-signed certificate

Create a 2048-bit RSA private key:
```shell
openssl genrsa -out domainname.key 2048
```

Then create the self-signed certificate (valid for 365 days, using sha256):
```shell
openssl req -new -x509 -nodes -sha256 -days 365 -key domainname.key > domainname.crt
```

At this point point you will be prompted a few questions,
see [Certificate data](#certificate-data) below.

### Creating a certificate with a CA

Create a 2048-bit RSA private key:
```shell
openssl genrsa -out domainname.key 2048
```

Then create a Certificate Signing Request (CSR):
```shell
openssl req -new -key domainname.key -out domainname.csr
```

See [Certificate data](#certificate-data) below for answering the certificate
questions.

After this is done, you should send the `domainname.csr` to your CA for
signing, and when the transaction is done you get the certificate from the CA.

Save the certificate as `dommainname.crt`.

### Certificate data

When creating a certificate, you are asked a series of questions, follow this
guide:
```
Two letter country code.
    Example: DE.

State or Province Name.
    Example: Bavaria

Locality Name.
    Example: Munich

Organization Name.
    Use your name if this certificate is not for any organization.

Organizational Unit Name.
    The name of your department within your organization, like sysadmin, etc.
    (can be left blank)

Common Name.
    The hostname of your server.
    Example: secure.extatic.org

Your e-mail address.
    Example: bob@example.com
```

## Configuring uhub

Now that you have your certificates generated, just set these configuration
values in your `uhub.conf` file:
```
tls_private_key="/path/to/domainname.key"
tls_certificate="/path/to/domainname.crt"
tls_enable=yes
# Uncomment the following if you want to require ADCS connections
#tls_require=yes
```

Now you can connect to the hub using the `adcs://` protocol handle.

## Using Keyprint

Now that you have TLS activated on your hub, some clients prefer to have the
fingerprint of the hub's certificate in the URL (also called keyprint).

You can use the `tools/generate_keyprint.sh` helper script to generate your
server's keyprint URL. Change the certificate path and `example.com:1234`
below to the certificate for your uhub server and the hostname:port users will
use to connect to your uhub server.
```shell
bash ./tools/generate_keyprint.sh "/path/to/domainname.crt" "example.com:1234"
```

The script will print out your hub's URL complete with the keyprint, E.g.
```
adcs://example.com:1234?kp=SHA256/ABCDEFGHIJKLMNOPQRSTUVWXYZ234567ABCDEFGHIJKLMNOPQRST
```
