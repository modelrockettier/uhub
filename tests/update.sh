#!/bin/bash

set -e

cd auto
../exotic init.tcc test*.tcc exit.tcc > test.c
cd ..
