CC=gcc

TARGET_EXEC ?= sys_backup

IDIR ?= include/
ODIR ?= ./build
SDIR ?= ./src

SRCS := $(shell find $(SDIR) -iname "*.c")
OBJS := $(SRCS:%=$(ODIR)/%.o)

CFLAGS = -Wall -g -I$(IDIR)

$(ODIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(ODIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean install

clean:
	$(RM) -r $(ODIR)

install:
	install $(ODIR)/$(TARGET_EXEC) /usr/local/bin/$(TARGET_EXEC)