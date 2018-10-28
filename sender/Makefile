EXEC=../sender.exe
SRCS=$(wildcard *.c) 
CPPS=$(wildcard *.cpp) 
LDFLAGS=-lrt -lpthread -lm

# Makefile common part
CC=gcc
LD=gcc
AR=ar
CFLAGS+=-pedantic -Wall -Wextra -g -c -O0 -std=c11 -D_XOPEN_SOURCE -D_POSIX_C_SOURCE=200809L
CPPFLAGS+=-pedantic -Wall -Wextra -g -c -O0 -std=c11
EXE=$(EXEC)
OBJDIR=.obj
OBJS+=$(addprefix $(OBJDIR)/, $(SRCS:.c=.o))
OBJS+=$(addprefix $(OBJDIR)/, $(CPPS:.c=.o))

$(OBJDIR)/%o: %c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%o: %cpp
	$(CC) $(CPPFLAGS) $< -o $@
	
.PHONY: all clean

all: $(OBJDIR)/ $(EXEC)

clean:
	rm -Rf .obj 
	rm -f $(EXEC) *~

$(EXEC): $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -o $@ 
			
$(OBJDIR)/:
	mkdir -p $(OBJDIR)


