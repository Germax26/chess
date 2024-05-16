#!/bin/sh

set -xe

clang -g -DPLATFORM_DESKTOP -fPIC -DSUPPORT_FILEFORMAT_FLAC=1 -I./raylib/raylib-5.0/src/external/glfw/include -Iexternal/glfw/deps/ming -DGRAPHICS_API_OPENGL_33  -c ./raylib/raylib-5.0/src/rcore.c -o ./build/raylib/macos/rcore.o
clang -g -DPLATFORM_DESKTOP -fPIC -DSUPPORT_FILEFORMAT_FLAC=1 -I./raylib/raylib-5.0/src/external/glfw/include -Iexternal/glfw/deps/ming -DGRAPHICS_API_OPENGL_33  -c ./raylib/raylib-5.0/src/raudio.c -o ./build/raylib/macos/raudio.o
clang -g -DPLATFORM_DESKTOP -fPIC -DSUPPORT_FILEFORMAT_FLAC=1 -I./raylib/raylib-5.0/src/external/glfw/include -Iexternal/glfw/deps/ming -DGRAPHICS_API_OPENGL_33 -x objective-c -c ./raylib/raylib-5.0/src/rglfw.c -o ./build/raylib/macos/rglfw.o
clang -g -DPLATFORM_DESKTOP -fPIC -DSUPPORT_FILEFORMAT_FLAC=1 -I./raylib/raylib-5.0/src/external/glfw/include -Iexternal/glfw/deps/ming -DGRAPHICS_API_OPENGL_33  -c ./raylib/raylib-5.0/src/rmodels.c -o ./build/raylib/macos/rmodels.o
clang -g -DPLATFORM_DESKTOP -fPIC -DSUPPORT_FILEFORMAT_FLAC=1 -I./raylib/raylib-5.0/src/external/glfw/include -Iexternal/glfw/deps/ming -DGRAPHICS_API_OPENGL_33  -c ./raylib/raylib-5.0/src/rshapes.c -o ./build/raylib/macos/rshapes.o
clang -g -DPLATFORM_DESKTOP -fPIC -DSUPPORT_FILEFORMAT_FLAC=1 -I./raylib/raylib-5.0/src/external/glfw/include -Iexternal/glfw/deps/ming -DGRAPHICS_API_OPENGL_33  -c ./raylib/raylib-5.0/src/rtext.c -o ./build/raylib/macos/rtext.o
clang -g -DPLATFORM_DESKTOP -fPIC -DSUPPORT_FILEFORMAT_FLAC=1 -I./raylib/raylib-5.0/src/external/glfw/include -Iexternal/glfw/deps/ming -DGRAPHICS_API_OPENGL_33  -c ./raylib/raylib-5.0/src/rtextures.c -o ./build/raylib/macos/rtextures.o
clang -g -DPLATFORM_DESKTOP -fPIC -DSUPPORT_FILEFORMAT_FLAC=1 -I./raylib/raylib-5.0/src/external/glfw/include -Iexternal/glfw/deps/ming -DGRAPHICS_API_OPENGL_33  -c ./raylib/raylib-5.0/src/utils.c -o ./build/raylib/macos/utils.o
ar -crs ./build/raylib/macos/libraylib.a ./build/raylib/macos/rcore.o ./build/raylib/macos/raudio.o ./build/raylib/macos/rglfw.o ./build/raylib/macos/rmodels.o ./build/raylib/macos/rshapes.o ./build/raylib/macos/rtext.o ./build/raylib/macos/rtextures.o ./build/raylib/macos/utils.o