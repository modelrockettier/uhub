#!/bin/bash

set -x
set -e


if [ "$TRAVIS" = true ]; then
	OS_NAME="${TRAVIS_OS_NAME:-$(uname -s)}"
	DIST=$TRAVIS_DIST
	BRANCH=$TRAVIS_BRANCH

elif [ "$CIRRUS_CI" = true ]; then
	unset OS
	OS_NAME=${CIRRUS_OS:-$(uname -s)}

	if [ -n "$CIRRUS_TAG" ]; then
		BRANCH=$CIRRUS_TAG
	elif [ -n "$CIRRUS_BRANCH" ]; then
		BRANCH=$CIRRUS_BRANCH
	elif [ -n "$CIRRUS_BASE_BRANCH" ]; then
		BRANCH=$CIRRUS_BASE_BRANCH
	fi

else
	echo "WARNING: Unknown CI" >&2
	OS_NAME=$(uname -s)
fi

case "$OS_NAME" in
	[Dd]arwin|osx)   OS_NAME=osx     ;;
	freebsd|FreeBSD) OS_NAME=freebsd ;;
	[Ll]inux)        OS_NAME=linux   ;;
	[Ww]indows)      OS_NAME=windows ;;
	*) echo "Unknown OS name: $OS_NAME" >&2; exit 6 ;;
esac

if [ -z "$CONFIG" ]; then
	echo "WARNING: No CONFIG set, building minimal" >&2
	CONFIG=minimal
fi

export BRANCH CONFIG DIST OS_NAME


if [ "$OS_NAME" = "linux" ]; then
    PACKAGES="cmake libsqlite3-dev make "

	case "${CONFIG}" in
		full) PACKAGES+=" git libssl-dev libsystemd-dev pkg-config" ;;
		deb)  PACKAGES+=" debhelper fakeroot git libssl-dev pkg-config" ;; # libsystemd-dev
		minimal) ;;
		docker) exit 0 ;;
		*)
			echo "Unknown config: ${CONFIG}" >&2
			exit 5 ;;
	esac

	sudo apt-get update -q
	sudo apt-get install -y --no-install-suggests --no-install-recommends $PACKAGES

elif [ "$OS_NAME" = "osx" ]; then
	case "${CONFIG}" in
		minimal|full) exit 0 ;;
		*)
			echo "Unknown config: ${CONFIG}" >&2
			exit 5 ;;
	esac

#	# The travis brew addon currently handles dependencies
#	brew update
#	brew install cmake openssl pkg-config sqlite

elif [ "$OS_NAME" = "freebsd" ]; then
    PACKAGES="cmake coreutils gmake llvm sqlite3"

	case "${CONFIG}" in
		full) PACKAGES+=" git openssl" ;;
		minimal) ;;
		*)
			echo "Unknown config: ${CONFIG}" >&2
			exit 5 ;;
	esac

	pkg install -y $PACKAGES

else
	echo "Unknown OS: ${OS_NAME}" >&2
	exit 6
fi
