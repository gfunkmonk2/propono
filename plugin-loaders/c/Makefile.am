# C plugin loader

loaderdir = $(libdir)/propono-1/plugin-loaders

INCLUDES = \
	-I$(top_srcdir) 						\
	$(PROPONO_CFLAGS) 						\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED_CFLAGS)					\
	-DPROPONO_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

loader_LTLIBRARIES = libcloader.la

NOINST_H_FILES = \
	propono-plugin-loader-c.h

libcloader_la_SOURCES = \
	propono-plugin-loader-c.c 	\
	$(NOINST_H_FILES)

libcloader_la_LDFLAGS = $(LOADER_LIBTOOL_FLAGS)
libcloader_la_LIBADD = $(PROPONO_LIBS)

-include $(top_srcdir)/git.mk
