AUTOMAKE_OPTIONS=foreign

ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS=$(BOOST_CPPFLAGS)
AM_LDFLAGS=$(BOOST_LDFLAGS)

EASYBMP_SRCS=EasyBMP.cpp EasyBMP.h EasyBMP_BMP.h EasyBMP_DataStructures.h EasyBMP_VariousBMPutilities.h

RESOURCE_SRCS=CLUTResource.h CLUTResource.cpp PICTResource.h PICTResource.cpp SndResource.h SndResource.cpp $(EASYBMP_SRCS)

EXTRA_DIST=atque.wxg atque.icns Atque-Info.plist EasyBMP_License.txt COPYING.txt atque.xcodeproj/project.pbxproj atque.rc atque.ico README.txt

if BUILD_ATQUEGUI
if MAKE_WINDOWS
bin_PROGRAMS=atques atquem Atque
else
bin_PROGRAMS=atques atquem atque
endif
else
bin_PROGRAMS=atques atquem
endif

atques_SOURCES=atques.cpp split.cpp split.h filesystem.h $(RESOURCE_SRCS) 

atquem_SOURCES=atquem.cpp merge.cpp merge.h filesystem.h $(RESOURCE_SRCS)

if BUILD_ATQUEGUI
ATQUE_SOURCES=atque.h atque.cpp split.cpp split.h merge.cpp merge.h filesystem.h $(RESOURCE_SRCS)
if MAKE_WINDOWS
atque-resources.o:
	@WX_RESCOMP@ -o atque-resources.o -I$(srcdir) $(srcdir)/atque.rc
Atque_LDADD=atque-resources.o
Atque_SOURCES=$(ATQUE_SOURCES)
else
atque_SOURCES=$(ATQUE_SOURCES)
endif
endif


