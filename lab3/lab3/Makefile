
# Makefile, ECE252  
# Yiqing Huang 

CC = gcc
CFLAGS = -Wall -g -std=c99 
LD = gcc
LDFLAGS = -g3 -fsanitize=address -std=c99
LDLIBS = -pthread -lcurl -lpng -lz  

LIB_UTIL = zutil.o crc.o catpng.o shm_stack.o
SRCS   = paster2.c crc.c zutil.c catpng.c shm_stack.c
OBJS   = paster2.o $(LIB_UTIL) 
TARGETS = paster2

all: ${TARGETS}

paster2: $(OBJS) 
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *~ *.d *.o $(TARGETS) 
