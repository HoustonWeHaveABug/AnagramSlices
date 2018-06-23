ANAGRAM_SLICES_C_FLAGS=-O2 -Wall -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wconversion -Wformat=2 -Winline -Wlong-long -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wno-import -Wpointer-arith -Wredundant-decls -Wshadow -Wstrict-prototypes -Wwrite-strings

anagram_slices: anagram_slices.o
	gcc -o anagram_slices anagram_slices.o

anagram_slices.o: anagram_slices.c anagram_slices.make
	gcc -c ${ANAGRAM_SLICES_C_FLAGS} -o anagram_slices.o anagram_slices.c

clean:
	rm -f anagram_slices anagram_slices.o
