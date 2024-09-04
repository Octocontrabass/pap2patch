CFLAGS=-Wall -Wextra -O2 `pkgconf --cflags zlib`
LDLIBS=`pkgconf --libs zlib`

extract:
