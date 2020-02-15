#!/bin/bash

# Exit codes
# 0: Success
#
# 5: Unknown config
# 6: Unknown OS
# 7: System does not have a working "which" command
#
# Anything else: Unknown error

set -x
set -e

quiet() { "$@" >/dev/null 2>&1; }
exists() { quiet which "$@"; }

# Check if we have a working "which"
if ! exists bash; then
	exists() { quiet type -P "$@"; }

	if ! exists bash; then
		echo "ERROR: 'which' does not work" >&2
		exit 7
	fi
fi

# Don't need sudo as root and if there's no sudo, try running without it
if [ "$UID" = 0 ] || ! exists sudo; then
	sudo() { "$@"; }
fi

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
	if exists apt-get; then
		PACKAGES="cmake libsqlite3-dev make"

		case "${CONFIG}" in
			deb)  PACKAGES="$PACKAGES build-essential debhelper fakeroot git libssl-dev pkg-config" ;; # libsystemd-dev
			full) PACKAGES="$PACKAGES git libssl-dev libsystemd-dev pkg-config" ;;
			minimal) ;;
			docker) exit 0 ;;
			*)
				echo "Unknown config: ${CONFIG}" >&2
				exit 5 ;;
		esac

		# Only install gcc if we don't already have a c compiler
		if ! exists cc; then
			PACKAGES="$PACKAGES gcc"
		fi

		sudo apt-get update -q
		sudo apt-get install -y --no-install-suggests --no-install-recommends $PACKAGES

	elif exists yum || exists dnf; then
		PACKAGES="cmake make sqlite-devel"

		case "${CONFIG}" in
			full) PACKAGES="$PACKAGES git openssl-devel pkgconfig systemd-devel" ;;
			rpm) PACKAGES="$PACKAGES git openssl-devel pkgconfig rpm-build systemd-devel" ;; # redhat-lsb-core rpm-devel
			minimal) ;;
			docker) exit 0 ;;
			*)
				echo "Unknown config: ${CONFIG}" >&2
				exit 5 ;;
		esac

		# Only install gcc if we don't already have a c compiler
		if ! exists cc; then
			PACKAGES="$PACKAGES gcc"
		fi

		if exists yum; then
			sudo yum install -y $PACKAGES
		else
			sudo dnf install -y $PACKAGES
		fi

	elif exists apk; then
		PACKAGES="util-linux sqlite-dev build-base cmake make"

		case "${CONFIG}" in
			full) PACKAGES="$PACKAGES git openssl-dev" ;;
			minimal) ;;
			docker) exit 0 ;;
			*)
				echo "Unknown config: ${CONFIG}" >&2
				exit 5 ;;
		esac

		# Only install gcc if we don't already have a c compiler
		if ! exists cc; then
			PACKAGES="$PACKAGES gcc"
		fi

		apk add --no-cache $PACKAGES

	else
		echo "WARNING: Unknown package manager." >&2
	fi

elif [ "$OS_NAME" = "osx" ]; then
	case "${CONFIG}" in
		minimal|full) exit 0 ;;
		*)
			echo "Unknown config: ${CONFIG}" >&2
			exit 5 ;;
	esac

	if [ "$TRAVIS" = true ]; then
		# The travis brew addon currently handles dependencies
		exit 0
	else
		brew update
		brew install cmake openssl pkg-config sqlite
	fi

elif [ "$OS_NAME" = "freebsd" ]; then
    PACKAGES="cmake coreutils sqlite3"

	case "${CONFIG}" in
		full) PACKAGES="$PACKAGES openssl" ;; # git is borked in pkg
		minimal) ;;
		*)
			echo "Unknown config: ${CONFIG}" >&2
			exit 5 ;;
	esac

	# Only install llvm if we don't already have a c compiler
	if ! exists cc; then
		PACKAGES="$PACKAGES llvm"
	fi

	# Only install gmake if we don't already have a make installed
	if ! exists make; then
		PACKAGES="$PACKAGES gmake"
	fi

	pkg install -y $PACKAGES

else
	echo "Unknown OS: ${OS_NAME}" >&2
	exit 6
fi
