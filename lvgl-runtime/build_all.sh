#!/bin/bash

cd v8.4.0/build
emcmake cmake ..
make -j8
cd ../..

cd v9.2.2/build
emcmake cmake ..
make -j8
cd ../..

cd v9.3.0/build
emcmake cmake ..
make -j8
cd ../..

cd v9.4.0/build
emcmake cmake ..
make -j8
cd ../..
