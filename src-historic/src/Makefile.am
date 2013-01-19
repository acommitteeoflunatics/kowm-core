bin_PROGRAMS = mavosxwm

mavosxwm_SOURCES = Atoms.cpp Atoms.h BlitBox.cpp BlitBox.h Client.cpp Client.h \
  Cursors.cpp Cursors.h Manager.cpp Manager.h mavosxwm.cpp Menu.cpp Menu.h \
  Workspace.cpp Workspace.h

mavosxwm_LDADD = @X_LIBS@ -lX11 @XFT_LIBS@ @GLIB_LIBS@
INCLUDES = @X_CFLAGS@ @XFT_CFLAGS@ @GLIB_CFLAGS@
