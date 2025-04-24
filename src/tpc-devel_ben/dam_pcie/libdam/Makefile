CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O2 -g
CFLAGS += -I../common
LDFLAGS = -shared
RM = rm -f
TARGET_LIB = libdam.so 

SRCS = libdam.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
		$(CC) ${LDFLAGS} -o $@ $^

$(SRCS:.c=.d):%.d:%.c
		$(CC) $(CFLAGS) -MM $< >$@

include $(SRCS:.c=.d)

clean:
	-${RM} -f ${TARGET_LIB} ${OBJS} $(SRCS:.c=.d)
