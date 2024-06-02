CC = gcc
CFLAGS = -pthread -Wall -Wextra -Wpedantic -Wformat=2 -Wcast-align -Wconversion -Wnull-dereference -Wno-incompatible-pointer-types
SRCDIR = ./code
OUTDIR = ./output

all: bdd server client

bdd: $(SRCDIR)/bdd.c
	$(CC) $(CFLAGS) -o $(OUTDIR)/bdd $(SRCDIR)/bdd.c

server: $(SRCDIR)/server.c $(SRCDIR)/bdd.c
	$(CC) $(CFLAGS) -o $(OUTDIR)/bdd $(SRCDIR)/bdd.c && $(CC) $(CFLAGS) -o $(OUTDIR)/server $(SRCDIR)/server.c

client: $(SRCDIR)/client.c
	$(CC) $(CFLAGS) -o $(OUTDIR)/client $(SRCDIR)/client.c

run_server: $(OUTDIR)/server $(OUTDIR)/bdd
	cd $(OUTDIR) && ./server

run_client: $(OUTDIR)/client
	cd $(OUTDIR) && ./client

clean:
	rm -f $(OUTDIR)/bdd $(OUTDIR)/server $(OUTDIR)/client
