# tools
CC := gcc
CXX := g++
RM := rm -f
CP := cp

# flags
CFLAGS := -std=gnu99 -w -g -O0 -z execstack -fno-stack-protector -U-FORTIFY-SOURCE

# sources
targets := vulnerable_01

# gmake magic
.PHONY:  all clean

#targets
all: $(targets)

clean:
	$(RM) $(targets)
