# Makefile, ECE252  
# Yiqing Huang <yqhuang@uwaterloo.ca>

CC = gcc 
CFLAGS_XML2 = $(shell xml2-config --cflags)
CFLAGS_CURL = $(shell curl-config --cflags)
CFLAGS = -Wall $(CFLAGS_XML2) $(CFLAGS_CURL) -std=gnu99 -g -D_GNU_SOURCE
LD = gcc
LDFLAGS = -std=gnu99 -g -D_GNU_SOURCE
LDLIBS_XML2 = $(shell xml2-config --libs)
LDLIBS_CURL = $(shell curl-config --libs)
LDLIBS = $(LDLIBS_XML2) $(LDLIBS_CURL) -pthread -lpng -lz -lssl -lm

SRCS   = findpng3.c curl_helper.c hashmap.c node.c
OBJS   = findpng3.o curl_helper.o hashmap.o node.o
TARGETS= findpng3

all: ${TARGETS}

findpng3: $(OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *~ *.d *.o $(TARGETS) *.png *.html
