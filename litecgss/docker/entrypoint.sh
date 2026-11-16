#!/bin/sh
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug .

cmake --build .

rm /tmp/.X1-lock

Xvfb :1 -screen 0 800x600x24+32 & fluxbox & x11vnc -forever
