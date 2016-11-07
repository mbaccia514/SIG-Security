# tools
CC := gcc
RM := rm -f

# flags
CFLAGS := -std=c99 -w -g -O0
VULNFLAGS := -zexecstack -fno-stack-protector -U_FORTIFY_SOURCE

# sources
targets := VClient VServer SClient SServer evil
vtargets := VClient VServer
stargets := SClient SServer

# src directory
sdir := src
bdir := bin

# dependencies
SDEPS := $(sdir)/simple_chat.h
VDEPS := $(sdir)/vuln_simple_chat.h

.PHONY: all secure vuln clean

# recipes 
all: $(targets)

secure: $(stargets)

vuln: $(vtargets)

evil: $(sdir)/evil_simple_chat_client.c
	$(CC) $(CFLAGS) -o $(bdir)/$@ $< $(SDEPS)

VClient: $(sdir)/simple_chat_client.c
	$(CC) $(CFLAGS) $(VULNFLAGS) -o $(bdir)/$@ $< $(VDEPS)

VServer: $(sdir)/vuln_simple_chat_server.c
	$(CC) $(CFLAGS) $(VULNFLAGS) -o $(bdir)/$@ $< $(VDEPS)

SClient: $(sdir)/simple_chat_client.c
	$(CC) $(CFLAGS) -o $(bdir)/$@ $< $(SDEPS)

SServer: $(sdir)/simple_chat_server.c
	$(CC) $(CFLAGS) -o $(bdir)/$@ $< $(SDEPS)

clean:
	$(RM) $(addprefix $(bdir)/, $(targets))
