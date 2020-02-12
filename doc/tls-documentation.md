# Setting up a TLS/SSL hub

## About certificates

Before you can setup an TLS protected hub, you must create an TLS certificate for the hub.

NOTE: uhub must be compiled with `SSL_SUPPORT` enabled in order for this to work
(enabled by default, but not for Windows).

## Configuring uhub

If you have your certificates ready, just set these configuration values in uhub.conf file:
```
  tls_private_key="/path/to/domainname.key"
  tls_certificate="/path/to/domainname.crt"
  tls_enable=yes
  tls_require=yes
```

Now you can connect to the hub using the adcs:// protocol handle.

## Creating certificates

### Creating a self-signed certificate

To create self-signed certificates with an 2048 bits RSA private key using the following command:
```
openssl genrsa -out domainname.key 2048
```

Then create the certificate (valid for 365 days, using sha256):
```
openssl req -new -x509 -nodes -sha256 -days 365 -key domainname.key > domainname.crt
```

At this point point you will be prompted a few questions, see the section Certificate data below.

## Creating a certificate with a CA

Create an 2048 bits RSA private key using the following command:
```
openssl genrsa -out domainname.key 2048
```

Then create a Certificate Signing Request (csr):
```
openssl req -new -key domainname.key -out domainname.csr
```

See the "Certificate data" section below for answering the certificate questions.

After this is done, you should send the domainname.csr to your CA for signing, and when the transaction is done you get the certificate from the CA.

Save the certificate as dommainname.crt.

## Certificate data

When creating a certificate, you are asked a series of questions, follow this guide:
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

## Giving fingerprint

Now that you have tls activated on your hub, you may have to share the certificate fingerprint to your hub user:

Find it by using this commandline:
```
openssl x509 -noout -fingerprint -sha256 < "/path/to/domainname.crt" \
	| cut -d '=' -f 2 | tr -dc "[A-F][0-9]" | xxd -r -p | base32 | tr -d "="
```

And give your full uhub address:

adc://localhost:1511?kp=SHA256/THE_VALUE_RETURNED_BY_COMMANDLINE_ABOVE
