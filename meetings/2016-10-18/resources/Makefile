##############################################################################
#   WARNING: If you change how the targets are compiled in any way, your     #
#            exploits may not work when we test them. So don't do that!      #
##############################################################################

# tools
CC := gcc
CXX := g++
RM := rm -f
CP := cp

# flags
CFLAGS := -std=gnu99 -w -g -O0 -zexecstack -fno-stack-protector -U_FORTIFY_SOURCE

# sources
targets := vulnerable_01

# gmake magic
.PHONY:  all clean install

#targets
all: $(targets)

clean:
	$(RM) $(targets)
