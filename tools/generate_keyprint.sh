#!/bin/bash

OPENSSL=/usr/bin/openssl
NAME=certificate
CERTIFICATE="$1"

if [ ! -x ${OPENSSL} ]; then
	echo "Cannot locate the openssl utility: ${OPENSSL}"
	exit 1
fi

if [ -z "${CERTIFICATE}" ]; then
	echo "Usage: generate_keyprint.sh <certificate>"
	exit 2
elif [ ! -f "${CERTIFICATE}" ]; then
	echo "Cannot load the certificate: ${CERTIFICATE}"
	exit 2
fi

set -e
set -o pipefail

KP=$(${OPENSSL} x509 -noout -fingerprint -sha256 -in "${CERTIFICATE}" \
	| cut -d '=' -f 2 \
	| tr -dc "[A-F][0-9]" \
	| xxd -r -p \
	| base32 \
	| tr -d "=")

printf "adcs://localhost:1511?kp=SHA256/%s\n" "$KP"
