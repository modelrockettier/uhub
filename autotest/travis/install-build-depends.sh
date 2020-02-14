#!/bin/bash

set -x
set -e

case "${CONFIG}" in
	deb|docker|full|minimal) ;;
	*)
		echo "Unknown config: ${CONFIG}" >&2
		exit 5 ;;
esac

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
	case "${CONFIG}" in
		full) EXTRA="git libssl-dev libsystemd-dev pkg-config" ;;
		deb)  EXTRA="debhelper fakeroot git libssl-dev pkg-config" ;; # libsystemd-dev
		docker) exit 0 ;;
	esac

	sudo apt-get update -q
	sudo apt-get install -y cmake make libsqlite3-dev $EXTRA

#elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
#	brew update
#	brew install cmake openssl pkg-config sqlite

else
	echo "Unknown OS: ${TRAVIS_OS_NAME}" >&2
	exit 6
fi
