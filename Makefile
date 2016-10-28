all:
	gcc -o rendertext rendertext.c                    \
      -lfreetype -lpng -licucore -licui18n          \
      -licuuc -licudata -lharfbuzz -lharfbuzz-icu   \
      -I/usr/local/opt/freetype/include/freetype2/  \
      -I/usr/local/opt/icu4c/include/               \
      -L/usr/local/opt/freetype/lib/                \
      -L/usr/local/opt/icu4c/lib
