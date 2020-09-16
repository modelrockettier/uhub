#!/bin/sh

export LC_ALL=C

# Try to sort the env vars by name
ENV=$(env -0 2>/dev/null | sort -z | sed -z 's/\n/\n /g' | tr '\0' '\n')

if [ -n "${ENV}" ]; then
	echo "${ENV}"
else
	env
fi
