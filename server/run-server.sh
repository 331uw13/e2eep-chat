#!/bin/bash

make clean
make -j4 && ./e2eep-server

