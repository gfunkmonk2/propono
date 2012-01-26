NULL = 

INCLUDES =                                  \
        -I$(top_srcdir)                     \
        -I$(top_builddir)                   \
        -I$(top_srcdir)/propono             \
        -I$(top_builddir)/propono           \
        $(PROPONO_CFLAGS)                   \
        $(NULL)

noinst_LTLIBRARIES = libview.la

libview_la_SOURCES = \
	autoDrawer.h autoDrawer.c \
	drawer.h drawer.c \
	ovBox.h ovBox.c \
	$(NULL)

-include $(top_srcdir)/git.mk
