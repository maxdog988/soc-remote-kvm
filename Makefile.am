ACLOCAL_AMFLAGS = -I m4

SUBDIRS=libvncserver
DIST_SUBDIRS=libvncserver
EXTRA_DIST = CMakeLists.txt rfb/rfbconfig.h.cmake

if HAVE_POLEG
SUBDIRS+=nuvoton
DIST_SUBDIRS+=nuvoton
endif

bin_SCRIPTS = libvncserver-config

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libvncserver.pc

includedir=$(prefix)/include/rfb

include_HEADERS=rfb/rfb.h rfb/rfbconfig.h rfb/rfbproto.h \
	rfb/keysym.h rfb/rfbregion.h rfb/rfbclient.h

$(PACKAGE)-$(VERSION).tar.gz: dist

if HAVE_RPM
# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE).spec
	cp $(PACKAGE)-$(VERSION).tar.gz @RPMSOURCEDIR@
	rpmbuild -ba $(PACKAGE).spec
endif

t:
	$(MAKE) -C test test

