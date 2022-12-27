clang main.c glad_gl.c -I inc -Ofast -lglfw -lm -o car
strip --strip-unneeded car
upx --lzma --best car