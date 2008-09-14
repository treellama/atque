AUTOMAKE_OPTIONS=foreign

ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS=$(BOOST_CPPFLAGS)
AM_LDFLAGS=$(BOOST_LDFLAGS)

EASYBMP_SRCS=EasyBMP.cpp EasyBMP.h EasyBMP_BMP.h EasyBMP_DataStructures.h EasyBMP_VariousBMPutilities.h

RESOURCE_SRCS=CLUTResource.h CLUTResource.cpp PICTResource.h PICTResource.cpp SndResource.h SndResource.cpp $(EASYBMP_SRCS)

EXTRA_DIST=atque.wxg atque.icns Atque-Info.plist EasyBMP_License.txt COPYING.txt atque.xcodeproj/project.pbxproj

if BUILD_ATQUEGUI
bin_PROGRAMS=atques atquem atque
else
bin_PROGRAMS=atques atquem
endif

atques_SOURCES=atques.cpp split.cpp split.h $(RESOURCE_SRCS) 
atques_LDADD=$(BOOST_FILESYSTEM_LIB)

atquem_SOURCES=atquem.cpp merge.cpp merge.h $(RESOURCE_SRCS)
atquem_LDADD=$(BOOST_FILESYSTEM_LIB)

if BUILD_ATQUEGUI
atque_SOURCES=atque.h atque.cpp split.cpp split.h merge.cpp merge.h  $(RESOURCE_SRCS)
atque_LDADD=$(BOOST_FILESYSTEM_LIB)
endif


