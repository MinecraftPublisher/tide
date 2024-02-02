all: clean tide

tide: tide.c file.h
	clang tide.c -o tide -O0 -g

clean:
	rm -rf tide tide.dSYM