# vnc plugin
plugindir = $(PROPONO_PLUGINS_LIBS_DIR)

INCLUDES = \
	-I$(top_srcdir) 				\
	$(VNC_CFLAGS) 					\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)	

plugin_LTLIBRARIES = libvnc.la

libvnc_la_SOURCES = 						\
	propono-vnc-plugin.h propono-vnc-plugin.c		\
	propono-vnc-connection.h propono-vnc-connection.c	\
	propono-vnc-tab.h propono-vnc-tab.c			\
	propono-vnc-listener.h propono-vnc-listener.c		\
	propono-vnc-listener-dialog.h propono-vnc-listener-dialog.c \
	propono-vnc-tunnel.h propono-vnc-tunnel.c

if SELF_IFADDRS
libvnc_la_SOURCES += if/ifaddrs.h if/getifaddrs.c
endif

libvnc_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libvnc_la_LIBADD  = $(VNC_LIBS)

uidir = $(PROPONO_PLUGINS_DATA_DIR)/vnc
ui_DATA = vnc.ui

plugin_in_files = vnc.propono-plugin.desktop.in

vnc.propono-plugin: vnc.propono-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.propono-plugin.desktop.in=.propono-plugin)

EXTRA_DIST = $(ui_DATA) $(plugin_in_files)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)


-include $(top_srcdir)/git.mk
