IDIR =include# directory for header files
SDIR =src# source directory
ODIR =obj# object file directory
BDIR =bin# bin exec file directory
CC = gcc# C compiler
# CC = g++# C++ compiler
LDFLAGS= # Compiler flags i.e. -lm
CFLAGS = -std=c99 -Wall -Wshadow -Werror -I$(IDIR)# C compiler flags
# CFLAGS = -Wall -Wshadow -Werror# C++ compiler flags

# Getting the list of header files
HEADERS = $(wildcard $(IDIR)/*.h)

all: server client coordinator cashier

# For generating the object files
$(ODIR)/%.o: $(SDIR)/%.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(SDIR)/server.c $(SDIR)/common.c $(IDIR)/common.h $(ODIR)/server.o $(ODIR)/common.o
	@mkdir -p $(BDIR)
	$(CC) -o $(BDIR)/$@ $(ODIR)/server.o $(ODIR)/common.o $(CFLAGS)

client: $(SDIR)/client.c $(SDIR)/common.c $(IDIR)/common.h $(ODIR)/client.o $(ODIR)/common.o
	@mkdir -p $(BDIR)
	$(CC) -o $(BDIR)/$@ $(ODIR)/client.o $(ODIR)/common.o $(CFLAGS)

coordinator: $(SDIR)/coordinator.c $(SDIR)/common.c $(IDIR)/common.h $(ODIR)/coordinator.o $(ODIR)/common.o
	@mkdir -p $(BDIR)
	$(CC) -o $(BDIR)/$@ $(ODIR)/coordinator.o $(ODIR)/common.o $(CFLAGS)

cashier: $(SDIR)/cashier.c $(SDIR)/common.c $(IDIR)/common.h $(ODIR)/cashier.o $(ODIR)/common.o
	@mkdir -p $(BDIR)
	$(CC) -o $(BDIR)/$@ $(ODIR)/cashier.o $(ODIR)/common.o $(CFLAGS)

# Any file with the name clean will not interrupt the cmd clean
.PHONY: clean clean-obj clean-build

clean: clean-obj clean-build

clean-obj:
	rm -rf $(ODIR) *.DS_Store

clean-build:
	rm -rf $(BDIR)
