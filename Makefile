all: tide

tide: clean tide.c file.h
	clang tide.c -o tide -O0 -g

clean:
	rm -rf tide tide.dSYM

run: tide
	./tide test.txt

debug: tide
	lldb ./tide test.txt