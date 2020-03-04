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


# Test creating the docker image
if [ "${CONFIG}" = "docker" ]; then
	docker build -t uhub:$(uname -m) .

# Test creating the debian package
elif [ "${CONFIG}" = "deb" ]; then
	dpkg-buildpackage -us -uc -b -jauto

	du -shc ../uhub*.deb

	sudo dpkg --install ../uhub*.deb

	PLUGIN_DIR=$(perl -ne 'm@^#?plugin (/usr/lib.*/uhub)/mod_.*\.so@ and print $1,"\n" and exit 0' /etc/uhub/plugins.conf)
	if [ -z "${PLUGIN_DIR}" ]; then
		echo "Plugin dir not found" >&2
		exit 2
	fi

	du -shc /etc/uhub/* /usr/bin/uhub* "${PLUGIN_DIR}"/*

# Test the vanilla cmake build+install
elif [ "${CONFIG}" = "full" ] || [ "${CONFIG}" = "minimal" ]; then
	rm -rf builddir
	mkdir builddir
	cd builddir

	CMAKEOPTS_all=".."

	# Config-specific cmake options
	CMAKEOPTS_minimal="-DCMAKE_BUILD_TYPE=Release -DLOWLEVEL_DEBUG=OFF
	                   -DSSL_SUPPORT=OFF -DADC_STRESS=OFF"

	CMAKEOPTS_full="-DCMAKE_BUILD_TYPE=Debug -DLOWLEVEL_DEBUG=ON
	                -DSSL_SUPPORT=ON -DUSE_OPENSSL=ON -DADC_STRESS=ON"

	# OS-specific cmake options
	CMAKEOPTS_linux="  -DCMAKE_INSTALL_PREFIX=/usr -DPLUGIN_DIR=/usr/lib/uhub"
	CMAKEOPTS_freebsd="-DCMAKE_INSTALL_PREFIX=/usr -DPLUGIN_DIR=/usr/lib/uhub"

	# Config and OS specific cmake options
	CMAKEOPTS_full_linux="-DSYSTEMD_SUPPORT=ON"
	CMAKEOPTS_full_osx="-DOPENSSL_ROOT_DIR=/usr/local/opt/openssl"

	# Grab the cmake options for this specific OS + config combination
	CMAKEOPTS=""
	for i in CMAKEOPTS_{all,$CONFIG,$OS_NAME,${CONFIG}_${OS_NAME}}; do
		CMAKEOPTS+=" ${!i}"
	done

	# If the tests fail, print the test output to the logs to help debugging
	export CTEST_OUTPUT_ON_FAILURE=1

	cmake ${CMAKEOPTS}

	make VERBOSE=1 -j3
	du -shc autotest-bin mod_*.so uhub uhub-admin uhub-passwd
	make test

	if [ "$OS_NAME" = "linux" ] || [ "$OS_NAME" = "freebsd" ]; then
		sudo make install
		du -shc /etc/uhub/* /usr/bin/uhub* /usr/lib/uhub/*
	fi

else
	echo "Unknown config: ${CONFIG}" >&2
	exit 5
fi

