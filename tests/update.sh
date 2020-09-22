#!/bin/bash

# Try to cd into the tests directory to start
DIR=$(dirname "$0")
if [ "$DIR" != . ] && [ "$DIR" != "$PWD" ]; then
	cd "$DIR" >/dev/null
fi

set -e

for i in auto passwd; do
	echo "$i"
	pushd "$i" >/dev/null

	# Only update test.c if it changes. Write to a temp file first and
	# compare the two files. This keeps the original test.c's mod time
	# intact if it doesn't change.

	../exotic init.tcc test*.tcc exit.tcc > .test.c
	if cmp -s .test.c test.c; then
		rm -f .test.c
	else
		echo "$i/test.c updated"
		mv .test.c test.c
	fi

	popd >/dev/null
done
