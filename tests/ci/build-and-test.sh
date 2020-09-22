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

elif [ "$CIRRUS_CI" = true ]; then
	unset OS
	OS_NAME=${CIRRUS_OS:-$(uname -s)}

else
	echo "WARNING: Unknown CI" >&2
	OS_NAME=$(uname -s)
fi

OS_LOWER=$(tr '[:upper:]' '[:lower:]' <<<"$OS_NAME")
case "$OS_LOWER" in
	darwin)                    OS_NAME=osx       ;;
	freebsd|linux|osx|windows) OS_NAME=$OS_LOWER ;;

	*) echo "Unknown OS name: $OS_NAME" >&2; exit 6 ;;
esac

if [ -z "$CONFIG" ]; then
	echo "WARNING: No CONFIG set, building minimal" >&2
	CONFIG=minimal
fi

export CONFIG OS_NAME

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


ls_deb() {
	du -shc "$1"

	#files=$(dpkg -c "$1")
	#grep -qE "(^|\s)/etc/uhub/." <<<"$files"
	#grep -qE "(^|\s)/usr/bin/uhub" <<<"$files"
	#grep -qE "(^|\s)/usr/lib" <<<"$files"
}

ls_rpm() {
	du -sh "$1"

	#files=$(rpm -q -l -p "$1")
	#grep -q "^/etc/uhub/." <<<"$files"
	#grep -q "^/usr/bin/uhub" <<<"$files"
	#grep -q "^/usr/lib/uhub/." <<<"$files"
}

ls_installed() {
	local conffile=/etc/uhub/plugins.conf
	local regex='^#?plugin (.*/)mod_[^/ ]+\.so( .*)?$'

	PLUGIN_DIR=$(grep -E "$regex" <"$conffile" \
		| sed -r "s@${regex}@\\1@" \
		| head -n 1)

	if [ -z "${PLUGIN_DIR}" ]; then
		echo "Plugin dir not found" >&2
		exit 2
	fi

	# Strip the trailing / if plugin dir isn't "/"
	if [ "$PLUGIN_DIR" != "/" ]; then
		PLUGIN_DIR=${PLUGIN_DIR/%\/}
	fi

	du -shc /etc/uhub/* /usr/bin/uhub* "${PLUGIN_DIR}"/*
}

print_test_logs() {
	local r=$?
	for log in *test*.log; do
		echo "========================================"
		if [ "$log" = "*test*.log" ]; then
			echo "= No test log files"
		else
			echo "==> $log <=="
			nofail cat "$log"
			echo "==> $log <=="
		fi
		echo "========================================"
	done
	return $r
}

build_and_test() {
	# If the tests fail, print the test output to the logs to help debugging
	export CTEST_OUTPUT_ON_FAILURE=1

	# Configure
	cmake ${CMAKEOPTS}

	# Build
	if [ "$OS_NAME" = "windows" ]; then
		VERBOSE=1 cmake --build . --config $BUILD_TYPE --target ALL_BUILD -j3
		du -shc */autotest-bin.exe */passwd-test.exe */mod_*.dll */uhub.exe */uhub-passwd.exe

	else
		make VERBOSE=1 -j3
		du -shc autotest-bin passwd-test mod_*.so uhub uhub-admin uhub-passwd
	fi

	# Test (print test logs on failure)
	trap print_test_logs EXIT
	if [ "$OS_NAME" = "windows" ]; then
		cmake --build . --config $BUILD_TYPE --target RUN_TESTS
	else
		make test
	fi
	trap - EXIT
}


# Test creating the docker image
if [ "${CONFIG}" = "docker" ]; then
	docker build -t uhub:$(uname -m) .

# Test creating the raw debian package
elif [ "${CONFIG}" = "dpkg-build" ]; then
	dpkg-buildpackage -us -uc -b -jauto

	ls_deb ../uhub_*.deb

	sudo dpkg -i ../uhub_*.deb

	ls_installed

# Test creating the deb and rpm packages with cmake
elif [ "${CONFIG}" = "deb" ] || [ "${CONFIG}" = "rpm" ]; then
	rm -rf build
	mkdir build
	cd build

	CMAKEOPTS=".. -DCMAKE_BUILD_TYPE=Release
	           -DSSL_SUPPORT=ON -DHARDENING=ON -DSYSTEMD_SUPPORT=ON"

	if [ "${CONFIG}" = "deb" ]; then
		CMAKEOPTS="${CMAKEOPTS} -DCPACK_GENERATOR=DEB"
	elif [ "${CONFIG}" = "rpm" ]; then
		CMAKEOPTS="${CMAKEOPTS} -DCPACK_GENERATOR=RPM"
	fi

	build_and_test

	make package

	if [ "${CONFIG}" = "deb" ]; then
		ls_deb uhub-*.deb
		sudo dpkg -i uhub-*.deb
	elif [ "${CONFIG}" = "rpm" ]; then
		ls_rpm uhub-*.rpm
		sudo rpm -i uhub-*.rpm
	fi

	ls_installed

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
	CMAKEOPTS_freebsd="-DCMAKE_INSTALL_PREFIX=/usr"
	CMAKEOPTS_linux="  -DCMAKE_INSTALL_PREFIX=/usr"
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

	build_and_test

	if [ "$OS_NAME" = "linux" ] || [ "$OS_NAME" = "freebsd" ]; then
		sudo make install
		ls_installed

	elif [ "$OS_NAME" = "osx" ]; then
		sudo make install
		PFX=/usr/local/opt/uhub
		du -shc ${PFX}/bin/uhub* ${PFX}/etc/* ${PFX}/lib/*

	elif [ "$OS_NAME" = "windows" ]; then
		# make install doesn't work on windows, so don't do anything here

		# Remove old cache files + empty directories
		find "${VCPKG_CACHE}" \( -type f -mtime +60 \) -exec rm -fv {} +
		find "${VCPKG_CACHE}" \( -type d -empty \) -exec rmdir -pv {} + 2>/dev/null

	else
		echo "Unknown platform" >&2
		exit 6
	fi

else
	echo "Unknown config: ${CONFIG}" >&2
	exit 5
fi
