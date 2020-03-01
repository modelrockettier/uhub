#!/bin/bash

set -x
set -e

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
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

elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
	case "${CONFIG}" in
		minimal|full) exit 0 ;;
		*)
			echo "Unknown config: ${CONFIG}" >&2
			exit 5 ;;
	esac

#	# The travis brew addon currently handles dependencies
#	brew update
#	brew install cmake openssl pkg-config sqlite

else
	echo "Unknown OS: ${TRAVIS_OS_NAME}" >&2
	exit 6
fi
