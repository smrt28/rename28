
SUBDIRS=gtest

AUTOMAKE_OPTIONS = foreign subdir-objects
ACLOCAL_AMFLAGS = -I aux
AM_CXXFLAGS=-std=c++0x

bin_PROGRAMS      = rename28 test28

#MODULES_LDFLAGS = -avoid-version -module -shared -export-dynamic


rename28_SOURCES = \
	src/main.cc src/escape.cc \
	src/escape.h src/error.h \
	src/hash.cc src/dir.cc \
	src/file.cc

rename28_CPPFLAGS = -Wall @REMOVE28_CFLAGS@
rename28_LDFLAGS = @REMOVE28_LIBS@


test28_CPPFLAGS = -I$(top_srcdir)/gtest/include
test28_LDADD = gtest/libgtest_main.a gtest/libgtest.a -lpthread -lcrypt
test28_SOURCES =\
	src/test.cc src/escape.cc

#rename28_LDADD   = -lcrypt


