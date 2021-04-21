CC=gcc

TARGET_EXEC ?= backup

IDIR ?= include/
ODIR ?= ./build
SDIR ?= ./src
PDIR ?= /usr/local/bin

SRCS := $(shell find $(SDIR) -iname "*.c")
OBJS := $(SRCS:%=$(ODIR)/%.o)

CFLAGS = -Wall -I$(IDIR)

$(ODIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(ODIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -r $(ODIR)

install:
	install $(ODIR)/$(TARGET_EXEC) $(PDIR)/$(TARGET_EXEC)

uninstall:
	$(RM) -r $(ODIR)
	$(RM) $(PDIR)/$(TARGET_EXEC)
