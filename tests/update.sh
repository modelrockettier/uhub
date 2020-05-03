#!/bin/bash

set -e

for i in auto passwd; do
	echo $i
	cd $i
	../exotic init.tcc test*.tcc exit.tcc > test.c
	cd ..
done
