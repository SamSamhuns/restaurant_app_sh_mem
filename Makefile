IDIR :=include# directory for header files
SDIR :=src# source directory
ODIR :=obj# object file directory
CC := gcc# C compiler
LDFLAGS := # Compiler flags i.e. -lm
UNAME := $(shell uname)# Check the OS

ifeq ($(UNAME), Linux)# Linux compilation
CFLAGS := -std=c99 -Wall -Wshadow -Werror -I$(IDIR) -D_XOPEN_SOURCE=500 -lrt -pthread# C compiler flags
endif
ifeq ($(UNAME), Darwin)# OSX compilation
CFLAGS := -std=c99 -Wall -Wshadow -Werror -I$(IDIR) -D_XOPEN_SOURCE=500# C compiler flags
endif

# Getting the list of header files
HEADERS = $(wildcard $(IDIR)/*.h)

all: server client coordinator cashier

# For generating the object files
$(ODIR)/%.o: $(SDIR)/%.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(SDIR)/server.c $(SDIR)/common.c $(IDIR)/common.h $(ODIR)/server.o $(ODIR)/common.o
	$(CC) -o $@ $(ODIR)/server.o $(ODIR)/common.o $(CFLAGS)

client: $(SDIR)/client.c $(SDIR)/common.c $(IDIR)/common.h $(ODIR)/client.o $(ODIR)/common.o
	$(CC) -o $@ $(ODIR)/client.o $(ODIR)/common.o $(CFLAGS)

coordinator: $(SDIR)/coordinator.c $(SDIR)/common.c $(IDIR)/common.h $(ODIR)/coordinator.o $(ODIR)/common.o
	$(CC) -o $@ $(ODIR)/coordinator.o $(ODIR)/common.o $(CFLAGS)

cashier: $(SDIR)/cashier.c $(SDIR)/common.c $(IDIR)/common.h $(ODIR)/cashier.o $(ODIR)/common.o
	$(CC) -o $@ $(ODIR)/cashier.o $(ODIR)/common.o $(CFLAGS)

# Any file with the name clean will not interrupt the cmd clean
.PHONY: clean clean-obj clean-build

clean: clean-obj clean-build clean-mac-fsys

clean-obj:
	rm -rf $(ODIR)

clean-mac-fsys:
	rm -rf *.DS_Store

clean-build:
	rm -rf server client coordinator cashier

clean-stats:
	rm -rf stats
