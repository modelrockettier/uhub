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

	du -shc /etc/uhub/ /usr/bin/uhub* "${PLUGIN_DIR}"

# Test the vanilla cmake build+install
elif [ "${CONFIG}" = "full" ] || [ "${CONFIG}" = "minimal" ]; then
	rm -rf builddir
	mkdir builddir
	cd builddir

	CMAKEOPTS="..
			   -DCMAKE_INSTALL_PREFIX=/usr
			   -DPLUGIN_DIR=/usr/lib/uhub"

	if [ "${CONFIG}" = "full" ]; then
		CMAKEOPTS="${CMAKEOPTS}
				   -DCMAKE_BUILD_TYPE=Debug
				   -DLOWLEVEL_DEBUG=ON
				   -DSSL_SUPPORT=ON
				   -DUSE_OPENSSL=ON
				   -DADC_STRESS=ON"
		if [ "$TRAVIS_OS_NAME" = "linux" ]; then
			CMAKEOPTS="${CMAKEOPTS}
					   -DSYSTEMD_SUPPORT=ON"
		fi
	else
		CMAKEOPTS="${CMAKEOPTS}
				   -DCMAKE_BUILD_TYPE=Release
				   -DLOWLEVEL_DEBUG=OFF
				   -DSSL_SUPPORT=OFF
				   -DADC_STRESS=OFF"
	fi

	cmake ${CMAKEOPTS}
	make VERBOSE=1 -j3

	make VERBOSE=1 -j3 autotest-bin
	./autotest-bin

	sudo make install
	du -shc /etc/uhub/ /usr/bin/uhub* /usr/lib/uhub/

else
	echo "Unknown config: ${CONFIG}" >&2
	exit 5
fi

