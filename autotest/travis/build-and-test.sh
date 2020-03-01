#!/bin/bash

set -x
set -e

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
	CMAKEOPTS_linux="-DCMAKE_INSTALL_PREFIX=/usr -DPLUGIN_DIR=/usr/lib/uhub"

	# Config and OS specific cmake options
	CMAKEOPTS_full_linux="-DSYSTEMD_SUPPORT=ON"
	CMAKEOPTS_full_osx="-DOPENSSL_ROOT_DIR=/usr/local/opt/openssl"

	# Grab the cmake options for this specific OS + config combination
	CMAKEOPTS=""
	for i in CMAKEOPTS_{all,$CONFIG,$TRAVIS_OS_NAME,${CONFIG}_${TRAVIS_OS_NAME}}; do
		CMAKEOPTS+=" ${!i}"
	done

	# If the tests fail, print the test output to the logs to help debugging
	export CTEST_OUTPUT_ON_FAILURE=1

	cmake ${CMAKEOPTS}

	make VERBOSE=1 -j3
	du -shc autotest-bin mod_*.so uhub uhub-admin uhub-passwd
	make test

	if [ "$TRAVIS_OS_NAME" = "linux" ]; then
		# make install doesn't work on mac/windows
		sudo make install
		du -shc /etc/uhub/* /usr/bin/uhub* /usr/lib/uhub/*
	fi

else
	echo "Unknown config: ${CONFIG}" >&2
	exit 5
fi

