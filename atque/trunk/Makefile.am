AUTOMAKE_OPTIONS=foreign

ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS=$(BOOST_CPPFLAGS)
AM_LDFLAGS=$(BOOST_LDFLAGS)

bin_PROGRAMS=atques atquem

atques_SOURCES=atques.cpp split.cpp split.h
atques_LDADD=$(BOOST_FILESYSTEM_LIB)

atquem_SOURCES=atquem.cpp merge.cpp merge.h
atquem_LDADD=$(BOOST_FILESYSTEM_LIB)

