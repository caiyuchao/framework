CFLAGS = -g -O0 -Wall -lpthread
bin_PROGRAMS = test
test_SOURCES = main.c
test_LDADD = $(LIBOBJS)
SUBDIRS = mm lib
LIBOBJS = mm/libmm.a lib/libetc.a
