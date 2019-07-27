#!/usr/bin/make -f
STRIP=strip
CC=gcc
ifeq (,$(wildcard /usr/bin/diet))
	DIET_PIECC := $(CC)
	DIET_PIELDFLAGS := -pie
else
	DIET_PIECC := /usr/bin/diet $(CC)
	DIET_PIELDFLAGS := -static-pie
endif
DIET_PIELDFLAGS += -s -Wl,-z,now
DIET_PIECFLAGS := -Wall -Os -fPIE -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables

fedora-30/opt/git-remote-xawsip: fedora-30/opt/git-remote-xawsip.c
	$(DIET_PIECC) $(DIET_PIELDFLAGS) $(DIET_PIECFLAGS) -o "$@" "$<" && strip "$@"
