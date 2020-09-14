#!/bin/bash
#
# Copyright (c) 2019, Kcchouette
# Copyright (c) 2020, Tim Schlueter
# SPDX-License-Identifier: GPL-3.0-or-later

NAME=certificate
CERT="$1"
HOST="${2:-localhost:1511}"

if ! openssl x509 -help &>/dev/null; then
	echo "Failed to run the openssl utility" >&2
	echo "Make sure it's in the \$PATH" >&2
	exit 1
fi

if [ -z "$CERT" ]; then
	echo "Usage: generate_keyprint.sh <certificate_file> [hostname:port]" >&2
	exit 2
fi

if [ ! -f "${CERT}" ]; then
	echo "Cannot load the certificate: ${CERT}"
	exit 2
fi

set -e
set -o pipefail

KP=$(openssl x509 -noout -fingerprint -sha256 -in "${CERT}" \
	| cut -d '=' -f 2 \
	| tr -dc "[A-F][0-9]" \
	| xxd -r -p \
	| base32 \
	| tr -d "=")

r=${PIPESTATUS[0]}
if [ $r -ne 0 ] || [ -z "$KP" ]; then
	echo "Failed to generate keyprint hash" >&2
	exit $((r ? r : 3))
fi

printf "adcs://%s?kp=SHA256/%s\n" "${HOST}" "$KP"
