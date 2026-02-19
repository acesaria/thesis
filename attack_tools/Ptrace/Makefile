# src/Makefile

# Directories
SDIR = src
CDIR = src/common
ODIR = bin

# Compiler flags
CC = gcc
CFLAGS = -Wall -Wextra -I$(CDIR)
LDFLAGS = -ldl

all: shellcode_injection lib_injection lib_rev hollow victim

shellcode_injection:
	$(CC) $(CFLAGS) $(SDIR)/shellcode_inject_fa.c $(CDIR)/*.c -o $(ODIR)/shellcode_inject_fa
	$(CC) $(CFLAGS) $(SDIR)/shellcode_inject_rip.c $(CDIR)/*.c -o $(ODIR)/shellcode_inject_rip

lib_injection:
	$(CC) $(CFLAGS) $(SDIR)/lib_inject.c $(CDIR)/*.c -o $(ODIR)/lib_inject $(LDFLAGS)

hollow:
	$(CC) $(CFLAGS) $(SDIR)/hollow.c $(CDIR)/*.c -o $(ODIR)/hollow

victim:
	$(CC) $(SDIR)/victim.c -o $(ODIR)/victim

lib_rev:
	$(CC) -shared -fPIC $(SDIR)/lib_rev.c -o $(ODIR)/lib_rev.so

clean:
	rm -rf $(ODIR)/*
	rm -f $(CDIR)/*.o