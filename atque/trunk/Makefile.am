AUTOMAKE_OPTIONS=foreign

ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS=$(BOOST_CPPFLAGS)
AM_LDFLAGS=$(BOOST_LDFLAGS)

RESOURCE_SRCS=CLUTResource.h CLUTResource.cpp PICTResource.h PICTResource.cpp SndResource.h SndResource.cpp

bin_PROGRAMS=atques atquem

atques_SOURCES=atques.cpp split.cpp split.h $(RESOURCE_SRCS)
atques_LDADD=$(BOOST_FILESYSTEM_LIB)

atquem_SOURCES=atquem.cpp merge.cpp merge.h $(RESOURCE_SRCS)
atquem_LDADD=$(BOOST_FILESYSTEM_LIB)

