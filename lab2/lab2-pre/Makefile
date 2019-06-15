
# Makefile, ECE252  
# Yiqing Huang 

CC = gcc
CFLAGS = -Wall -g -std=c99 
LD = gcc
LDFLAGS = -g -std=c99
LDLIBS = -pthread -lcurl -lpng 

LIB_UTIL = ../starter/png_util/zutil.o ../starter/png_util/crc.o
SRCS   = paster.c ../starter/png_util/crc.c ../starter/png_util/zutil.c
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
