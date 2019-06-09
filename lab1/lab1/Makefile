
# Makefile, ECE252  
# Yiqing Huang, 2018/11/04

CC = gcc       # compiler
CFLAGS = -Wall -g -std=c99 # compilation flg 
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with zlib

# For students 
LIB_UTIL = ../starter/png_util/zutil.o ../starter/png_util/crc.o
SRCS   = ../starter/png_util/crc.c ../starter/png_util/zutil.c findpng.c catpng.c
FINDOBJS   = findpng.o $(LIB_UTIL) 
CATOBJS   = catpng.o $(LIB_UTIL) 

TARGETS= findpng catpng

all: ${TARGETS}

findpng: $(FINDOBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

catpng: $(CATOBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *.d *.o $(TARGETS) 
