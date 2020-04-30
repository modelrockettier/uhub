#!/bin/bash

set -e

echo auto
cd auto
../exotic init.tcc test*.tcc exit.tcc > test.c
cd ..

echo passwd
cd passwd
../exotic init.tcc test*.tcc exit.tcc > test.c
cd ..
