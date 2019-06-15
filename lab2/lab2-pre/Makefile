
# Makefile, ECE252  
# Yiqing Huang 

CC = gcc
CFLAGS = -Wall -g -std=c99 
LD = gcc
LDFLAGS = -g -std=c99
LDLIBS = -pthread -lcurl -lpng -lz

LIB_UTIL = zutil.o crc.o catpng.o
SRCS   = paster.c crc.c zutil.c catpng.c
OBJS   = paster.o $(LIB_UTIL) 
TARGETS = paster 

all: ${TARGETS}

paster: $(OBJS) 
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *~ *.d *.o $(TARGETS) 
