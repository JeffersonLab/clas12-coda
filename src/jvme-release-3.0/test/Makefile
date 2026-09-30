#
# File:
#    Makefile
#
# Description:
#    Makefile for jvme library test programs
#
#
DEBUG	?= 1
QUIET	?= 1
#
ifeq ($(QUIET),1)
        Q = @
else
        Q =
endif


CROSS_COMPILE		=
CC			= $(CROSS_COMPILE)gcc
AR                      = ar
RANLIB                  = ranlib
INCS			= -I. -I../
CFLAGS			= -D_GNU_SOURCE -std=c99
CFLAGS			+= -L. -L../ -L../build -lrt -ljvme
ifeq ($(DEBUG),1)
	CFLAGS		+= -Wall -Wno-unused -g
endif

SRC			= $(wildcard *.c)
PROGS			= $(SRC:.c=)
#PROGS			= cmemTest

DEPDIR 			= .deps
DEPFLAGS		= -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d
DEPFILES		= $(SRC:%.c=$(DEPDIR)/%.d)

all: $(PROGS)

clean distclean:
	@rm -f $(PROGS) *~ $(OBJS) $(DEPFILES)

%: %.c
%: %.c $(DEPDIR)/%.d | $(DEPDIR)
	@echo " CC     $@"
	${Q}$(CC) $(DEPFLAGS)  -o $@ $< $(CFLAGS) $(INCS)

$(DEPDIR): ; @mkdir -p $@

$(DEPFILES):
include $(wildcard $(DEPFILES))


.PHONY: all clean distclean
