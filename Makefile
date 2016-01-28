.PHONY: all clean install dist

# Top directory for building complete system, fall back to this directory
ROOTDIR    ?= $(shell pwd)

VERSION = 1.0-beta1
NAME    = phytool
PKG     = $(NAME)-$(VERSION)
ARCHIVE = $(PKG).tar.xz
APPLETS = mv6tool

PREFIX ?= /usr/local/
LDLIBS  = 
CFLAGS  = -W -Wall -Wextra
CFLAGS += -g

objs = $(patsubst %.c, %.o, $(wildcard *.c))
hdrs = $(wildcard *.h)

%.o: %.c $(hdrs) Makefile
	@printf "  CC      $(subst $(ROOTDIR)/,,$(shell pwd)/$@)\n"
	@$(CROSS_COMPILE)$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@

phytool: $(objs)
	@printf "  CC      $(subst $(ROOTDIR)/,,$(shell pwd)/$@)\n"
	@$(CROSS_COMPILE)$(CC) $(LDLIBS) -o $@ $^

all: phytool

clean:
	@rm -f *.o
	@rm -f $(TARGET)

dist:
	@echo "Creating $(ARCHIVE), with $(ARCHIVE).md5 in parent dir ..."
	@git archive --format=tar --prefix=$(PKG)/ v$(VERSION) | xz >../$(ARCHIVE)
	@(cd .. && md5sum $(ARCHIVE) > $(ARCHIVE).md5)

install: phytool
	@cp phytool $(DESTDIR)/$(PREFIX)/bin/
	@for app in $(APPLETS); do \
		ln -sf phytool $(DESTDIR)/$(PREFIX)/bin/$$app; \
	done