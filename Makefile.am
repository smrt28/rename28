
#AUTOMAKE_OPTIONS = foreign subdir-objects
ACLOCAL_AMFLAGS = -I aux
AM_CXXFLAGS=-std=c++0x

bin_PROGRAMS      = rename28

#MODULES_LDFLAGS = -avoid-version -module -shared -export-dynamic


rename28_SOURCES = src/main.cc src/escape.cc src/escape.h src/error.h src/hash.cc

rename28_CPPFLAGS = -Wall @REMOVE28_CFLAGS@
rename28_LDFLAGS = @REMOVE28_LIBS@
#rename28_LDADD   = -lcrypt


