#!/bin/bash
#
# Copyright (C) 2007-2014, Jan Vidar Krey
# Copyright (C) 2020, Tim Schlueter
# SPDX-License-Identifier: GPL-3.0-or-later

# Exit codes
# 0: Success
#
# 2: Could not find plugin dir after deb install
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
	: ${VCPKG_ROOT="$HOME/vcpkg-${CONFIG}-${ARCH}"}
	export VCPKG_ROOT
	: ${VCPKG_CACHE="$HOME/vcpkg-cache-${CONFIG}-${ARCH}"}
	export VCPKG_CACHE
	export PATH="$PATH:$VCPKG_CACHE"

	# Add the installed DLL paths to PATH
	# Otherwise autotest-bin will fail to run
	for dir in "${VCPKG_ROOT}"/installed/*/bin; do
		if [ -d "$dir" ]; then
			export PATH="$PATH:$dir"
		fi
	done
fi


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

# Test creating the rpm package with cmake
elif [ "${CONFIG}" = "rpm" ]; then
	rm -rf build
	mkdir build
	cd build

	CMAKEOPTS=".. -DCMAKE_BUILD_TYPE=Release
	           -DSSL_SUPPORT=ON -DHARDENING=ON -DSYSTEMD_SUPPORT=ON
	           -DCMAKE_INSTALL_PREFIX=/usr -DPLUGIN_DIR=/usr/lib/uhub"

	# If the tests fail, print the test output to the logs to help debugging
	export CTEST_OUTPUT_ON_FAILURE=1

	cmake ${CMAKEOPTS}
	make VERBOSE=1 -j3

	du -shc autotest-bin passwd-test mod_*.so uhub uhub-admin uhub-passwd
	make test

	cpack -G RPM #-DCPACK_RPM_PACKAGE_DEBUG=1
	du -sh uhub-*.rpm

	#files=$(rpm -q -l -p uhub-*.rpm)

	#grep -q "^/etc/uhub/." <<<"$files"
	#grep -q "^/usr/bin/uhub" <<<"$files"
	#grep -q "^/usr/lib/uhub/." <<<"$files"

	sudo rpm -i uhub*.rpm
	du -shc /etc/uhub/* /usr/bin/uhub* /usr/lib/uhub/*

# Test the vanilla cmake build+install
elif [ "${CONFIG}" = "full" ] || [ "${CONFIG}" = "minimal" ]; then
	rm -rf build
	mkdir build
	cd build

	CMAKEOPTS_all=".."

	if [ "$CONFIG" = "minimal" ]; then
		export CFLAGS="-DDEBUG_TESTS"
		BUILD_TYPE=Release
	else
		BUILD_TYPE=Debug
	fi

	# Config-specific cmake options
	CMAKEOPTS_minimal="-DCMAKE_BUILD_TYPE=Release -DLOWLEVEL_DEBUG=OFF
	                   -DSSL_SUPPORT=OFF -DADC_STRESS=OFF"

	CMAKEOPTS_full="-DCMAKE_BUILD_TYPE=Debug -DLOWLEVEL_DEBUG=ON
	                -DSSL_SUPPORT=ON"

	# OS-specific cmake options
	CMAKEOPTS_freebsd="-DCMAKE_INSTALL_PREFIX=/usr -DPLUGIN_DIR=/usr/lib/uhub"
	CMAKEOPTS_linux="  -DCMAKE_INSTALL_PREFIX=/usr -DPLUGIN_DIR=/usr/lib/uhub"
	CMAKEOPTS_osx="    -DCMAKE_INSTALL_PREFIX=/usr/local/opt/uhub -DPLUGIN_DIR=/usr/local/opt/uhub/lib
	                   -DCONFIG_DIR=/usr/local/opt/uhub/etc -DLOG_DIR=/usr/local/opt/uhub/log"
	CMAKEOPTS_windows="-DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"

	if [ "$OS_NAME" = "windows" ] && [ -n "$ARCH" ]; then
		export VCPKG_DEFAULT_TRIPLET="${ARCH}-windows"
		export CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE="$ARCH"
		CMAKEOPTS_windows+=" -DVCPKG_TARGET_TRIPLET=$VCPKG_DEFAULT_TRIPLET
		                     -T host=${ARCH}"

		if [ "$ARCH" = "x86" ]; then
			CMAKEOPTS_windows+=" -A Win32"
		else
			CMAKEOPTS_windows+=" -A $ARCH"
		fi
	fi

	# Config and OS specific cmake options
	if [ -z "$NO_SYSTEMD" ]; then
		CMAKEOPTS_full_linux="-DSYSTEMD_SUPPORT=ON -DADC_STRESS=ON"
	fi
	CMAKEOPTS_full_freebsd="-DADC_STRESS=ON"
	CMAKEOPTS_full_osx="-DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DADC_STRESS=ON"

	# Grab the cmake options for this specific OS + config combination
	CMAKEOPTS=""
	for i in CMAKEOPTS_{all,$CONFIG,$OS_NAME,${CONFIG}_${OS_NAME}}; do
		CMAKEOPTS+=" ${!i} "
	done

	# If the tests fail, print the test output to the logs to help debugging
	export CTEST_OUTPUT_ON_FAILURE=1

	cmake ${CMAKEOPTS}

	ret=0
	if [ "$OS_NAME" = "windows" ]; then
		BUILD_ARGS="--build . --config $BUILD_TYPE"

		export VERBOSE=1
		cmake $BUILD_ARGS --target ALL_BUILD -j3
		unset VERBOSE

		du -shc */autotest-bin.exe */passwd-test.exe */mod_*.dll */uhub.exe */uhub-passwd.exe
		cmake $BUILD_ARGS --target RUN_TESTS || ret=$?
	else
		make VERBOSE=1 -j3

		du -shc autotest-bin passwd-test mod_*.so uhub uhub-admin uhub-passwd
		make test || ret=$?
	fi

	# Display the test log on failure before exiting
	if [ "$ret" != 0 ]; then
		set +x
		for log in *test*.log; do
			echo "========================================"
			echo "==> $log <=="
			nofail cat "$log"
			echo "==> $log <=="
			echo "========================================"
		done
		set -x
		exit $ret
	fi

	if [ "$OS_NAME" = "linux" ] || [ "$OS_NAME" = "freebsd" ]; then
		sudo make install
		du -shc /etc/uhub/* /usr/bin/uhub* /usr/lib/uhub/*
	elif [ "$OS_NAME" = "osx" ]; then
		sudo make install
		du -shc /usr/local/opt/uhub/{bin/uhub,etc/,lib/}*
	elif [ "$OS_NAME" = "windows" ]; then
		# make install doesn't work on windows, so don't do anything here

		# Remove old cache files
		find "${VCPKG_CACHE}" \( -type f -mtime +60 \) -exec rm -fv {} +

		# Remove empty directories
		find "${VCPKG_CACHE}" \( -type d -empty \) -exec rmdir -pv {} + 2>/dev/null
	else
		echo "Unknown platform" >&2
		exit 6
	fi

else
	echo "Unknown config: ${CONFIG}" >&2
	exit 5
fi
