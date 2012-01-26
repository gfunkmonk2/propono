# ssh plugin
plugindir = $(PROPONO_PLUGINS_LIBS_DIR)

INCLUDES = \
	-I$(top_srcdir) 				\
	$(SSH_CFLAGS) 					\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)	

plugin_LTLIBRARIES = libssh.la

libssh_la_SOURCES = 						\
	propono-ssh-plugin.h propono-ssh-plugin.c		\
	propono-ssh-connection.h propono-ssh-connection.c	\
	propono-ssh-tab.h propono-ssh-tab.c

libssh_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libssh_la_LIBADD  = $(SSH_LIBS)

plugin_in_files = ssh.propono-plugin.desktop.in

ssh.propono-plugin: ssh.propono-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.propono-plugin.desktop.in=.propono-plugin)

EXTRA_DIST = $(plugin_in_files)
CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)

-include $(top_srcdir)/git.mk
