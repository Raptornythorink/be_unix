CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wcast-align -Wconversion -Wsign-conversion -Wnull-dereference
SRCDIR = ./code
OUTDIR = ./output

all: bdd server client

bdd: $(SRCDIR)/bdd.c
	$(CC) $(CFLAGS) -o $(OUTDIR)/bdd $(SRCDIR)/bdd.c

server: $(SRCDIR)/server.c
	$(CC) $(CFLAGS) -o $(OUTDIR)/server $(SRCDIR)/server.c

client: $(SRCDIR)/client.c
	$(CC) $(CFLAGS) -o $(OUTDIR)/client $(SRCDIR)/client.c

run_server: $(OUTDIR)/server
	cd $(OUTDIR) && ./server

run_client: $(OUTDIR)/client
	cd $(OUTDIR) && ./client

clean:
	rm -f $(OUTDIR)/bdd $(OUTDIR)/server $(OUTDIR)/client
