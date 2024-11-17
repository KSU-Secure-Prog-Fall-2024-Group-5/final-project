#
# makefile for any
#

# this make-file has some subtle issues, mainly in regards to preprocessor
# stuff inside a few headers. so if you change preprocessor constants,
# do a full re-build lol.. it's not like it costs much time

CC = gcc
CC_FLAGS = -std=c99 -Wall -Wpedantic -Wextra -Werror -fsanitize=undefined
VALGRIND_FLAGS = --quiet --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=3 --error-exitcode=1

# ALL_OBJECTS = 

all: logappend logread

logappend: logappend.c common.h
	$(CC) $(CC_FLAGS) -o logappend logappend.c `pkg-config --cflags --libs libgcrypt`

logread: logread.c common.h
	$(CC) $(CC_FLAGS) -o logread logread.c `pkg-config --cflags --libs libgcrypt`

# -c for compiling but not linking
# -g for debugging with gdb...

clean:
	rm -f *.o
	rm -f ./logread ./logappend
