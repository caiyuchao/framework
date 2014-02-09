INCLUDE = ./include
AM_CPPFLAGS = -lpthread -g -O0 -I$(INCLUDE)
ACLOCAL_AMFLAGS = -I m4
bin_PROGRAMS = test
test_SOURCES = main.c
test_LDADD = $(LIBOBJS)
SUBDIRS = logging
LIBOBJS = logging/liblogging.a
