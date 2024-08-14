#!/bin/sh

set -xe

clang                                                         \
	-Wall -Wextra -g -I./build/ -I./raylib/raylib-5.0/src/    \
	-o ./build/main ./src/main.c                              \
	./build/raylib/macos/libraylib.a                          \
	-framework CoreVideo                                      \
	-framework IOKit                                          \
	-framework Cocoa                                          \
	-framework GLUT                                           \
	-framework OpenGL                                         \
	-lm -ldl -lpthread
