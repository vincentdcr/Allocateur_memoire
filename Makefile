CC=gcc

# uncomment to compile in 32bits mode (require gcc-*-multilib packages
# on Debian/Ubuntu)
#HOST32= -m32

CFLAGS+= $(HOST32) -Wall -Werror -std=c99 -g -D_GNU_SOURCE
CFLAGS+= -DDEBUG
CFLAGS+= -DMEMORY_SIZE=819200 #Pour passer de 8 Ko a 8 Mo de memoire
# pour tester avec ls
CFLAGS+= -fPIC
LDFLAGS= $(HOST32)
TESTS+=test_init
PROGRAMS=memshell memshell_write tests_allocateur $(TESTS)

.PHONY: clean all test_ls

all: $(PROGRAMS)
	for file in $(TESTS);do ./$$file; done

tests: $(PROGRAMS)
	./tests_allocateur

%.o: %.c
	$(CC) -c $(CFLAGS) -MMD -MF .$@.deps -o $@ $<

# dépendences des binaires
$(PROGRAMS) libmalloc.so: %: mem.o common.o

-include $(wildcard .*.deps)

# seconde partie du sujet
libmalloc.so: malloc_stub.o
	$(CC) -shared -Wl,-soname,$@ $^ -o $@

test_ls: libmalloc.so
	LD_PRELOAD=./libmalloc.so ls

# nettoyage
clean:
	$(RM) *.o $(PROGRAMS) libmalloc.so .*.deps
