#!/bin/bash


make clean
make -j4
cp -v ../server/public_key .
./e2eep-client public_key 331uw13

