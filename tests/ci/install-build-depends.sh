#!/bin/bash
#
# Copyright (C) 2007-2014, Jan Vidar Krey
# Copyright (C) 2020, Tim Schlueter
# SPDX-License-Identifier: GPL-3.0-or-later

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
nofail() { "$@" || true; }

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

if [ "$OS_NAME" = windows ]; then
	: ${VCPKG_ROOT="$HOME/vcpkg${CONFIG+-$CONFIG}${ARCH+-$ARCH}"}
	export VCPKG_ROOT
	export PATH="$PATH:$VCPKG_ROOT"
fi

stop_spinner() {
	# Stop the keep-alive spinner process
	nofail kill $SPINNER_PID
	nofail unset SPINNER_PID
	nofail trap - EXIT # Restore the default exit handler
}

start_spinner() {
	# Start a process that runs as a keep-alive to avoid travis quitting
	# if there is no output (for up to 20 mins)
	(
		set +x
		echo "Spinner started"
		for ((i=1; i<=20; i++)); do
			sleep 60
			kill -0 "$$" # Ensure the parent bash shell is still running
			echo "Still building ($i) ..." >&2
		done
	) &

	SPINNER_PID=$!
	# Stop the spinner if bash exits prematurely
	trap stop_spinner EXIT

	if ! kill -0 $SPINNER_PID; then
		echo "WARNING: Spinner $SPINNER_PID is not running" >&2
	fi
}


if [ "$OS_NAME" = "linux" ]; then
	if exists apt-get; then
		PACKAGES="cmake libsqlite3-dev make"

		case "${CONFIG}" in
			deb)  PACKAGES="$PACKAGES build-essential debhelper fakeroot git libssl-dev" ;;
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

	elif exists dnf || exists yum; then
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

		if exists dnf; then
			sudo dnf install -y --setopt=install_weak_deps=False $PACKAGES
		else
			sudo yum install -y $PACKAGES
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

elif [ "$OS_NAME" = "windows" ]; then
	CHOC_PKGS="cmake microsoft-build-tools visualstudio2017-workload-vctools"
	VC_PKGS="sqlite3"

	case "${CONFIG}" in
		minimal) ;;
		full) VC_PKGS="$VC_PKGS openssl" ;;
		*)
			echo "Unknown config: ${CONFIG}" >&2
			exit 5 ;;
	esac

	if ! exists git; then
		CHOC_PKGS="$CHOC_PKGS git"
	fi

	choco install -y $CHOC_PKGS

	# x86 or x64
	if [ -n "$ARCH" ]; then
		export VCPKG_DEFAULT_TRIPLET="${ARCH}-windows"
	fi

	# Download and install vcpkg if it's missing or if this script changes
	if [ ! -s "${VCPKG_ROOT}/uhub_ci.md5" ] || ! md5sum -c "${VCPKG_ROOT}/uhub_ci.md5"; then
		rm -rf $VCPKG_ROOT
		git clone https://github.com/Microsoft/vcpkg.git $VCPKG_ROOT
		pushd $VCPKG_ROOT

		# Try checking out the latest tag (don't care if these commands fail)
		set +e

		git config advice.detachedHead false
		TAG=$(git describe --abbrev=0 --tags) && git checkout "$TAG"
		git status

		# Exit on failure below here
		set -e

		# Reduce the build times by not building debug variants of the dependencies
		printf '\n%s\n' "set(VCPKG_BUILD_TYPE release)" \
			| quiet tee -a triplets/*windows*.cmake triplets/community/*windows*.cmake

		cmd "/C bootstrap-vcpkg.bat"
		popd

		md5sum tests/ci/install-build-depends.sh | tee "${VCPKG_ROOT}/uhub_ci.md5"
	fi

	# User-wide vcpkg integration
	nofail vcpkg integrate install

	# OpenSSL takes a while to build
	start_spinner

	for pkg in $VC_PKGS; do
		vcpkg install $pkg
	done

	stop_spinner

else
	echo "Unknown OS: ${OS_NAME}" >&2
	exit 6
fi
