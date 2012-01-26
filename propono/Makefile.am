SUBDIRS = view

NULL = 

INCLUDES = 						\
	-DBINDIR=\"$(bindir)\"			 	\
	-DDATADIR=\"$(datadir)\"			\
	-DPREFIX=\""$(prefix)"\" 			\
	-DSYSCONFDIR=\""$(sysconfdir)"\" 		\
	-DLIBDIR=\""$(libdir)"\" 			\
	-DPROPONO_DATADIR=\""$(datadir)/propono"\"	\
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"	\
	-DSSH_PROGRAM=\"$(SSH_PROGRAM)\"		\
	$(PROPONO_CFLAGS)				\
	$(AVAHI_CFLAGS)					\
	$(NULL)

NOINST_H_FILES = \
  bacon-message-connection.h \
  propono-enums.h \
  propono-marshal.h \
  propono-object-module.h \
  propono-plugin-info.h \
  propono-plugin-info-priv.h \
  propono-plugin-loader.h \
  propono-plugin-manager.h \
  propono-plugin-dialog.h \
  propono-plugins-engine.h \
  propono-window-private.h \
  propono-spinner.h \
  pty_open.h \
  $(NULL)

INST_H_FILES = \
  propono-app.h \
  propono-bacon.h \
  propono-bookmarks-entry.h \
  propono-bookmarks.h \
  propono-bookmarks-migration.h \
  propono-bookmarks-tree.h \
  propono-bookmarks-ui.h \
  propono-commands.h \
  propono-connect.h \
  propono-connection.h \
  propono-debug.h \
  propono-dirs.h \
  propono-fav.h \
  propono-notebook.h \
  propono-plugin.h \
  propono-plugin-loader.h \
  propono-prefs.h \
  propono-tab.h \
  propono-ui.h \
  propono-utils.h \
  propono-window.h \
  propono-dnd.h \
  propono-ssh.h \
  propono-cache-prefs.h \
  $(NULL)

headerdir = $(prefix)/include/propono-@PROPONO_API_VERSION@/propono
header_DATA = $(INST_H_FILES)

##############################
# The main library
##############################

noinst_LTLIBRARIES = libpropono.la

handwritten_sources = \
  bacon-message-connection.c  \
  propono-app.c \
  propono-bacon.c \
  propono-bookmarks.c \
  propono-bookmarks-entry.c \
  propono-bookmarks-migration.c \
  propono-bookmarks-tree.c \
  propono-bookmarks-ui.c \
  propono-commands.c \
  propono-connect.c \
  propono-connection.c \
  propono-debug.c \
  propono-dirs.c \
  propono-fav.c \
  propono-notebook.c \
  propono-object-module.c \
  propono-plugin.c \
  propono-plugin-info.c \
  propono-plugin-dialog.c \
  propono-plugin-loader.c \
  propono-plugin-manager.c \
  propono-plugins-engine.c \
  propono-prefs.c \
  propono-tab.c \
  propono-utils.c \
  propono-window.c \
  propono-spinner.c \
  pty_open.c \
  propono-ssh.c \
  propono-cache-prefs.c \
  $(NULL)

libpropono_la_SOURCES = \
  propono-enums.c \
  propono-marshal.c \
  $(handwritten_sources) \
  $(NOINST_H_FILES) \
  $(INST_H_FILES) \
  $(NULL)

if AVAHI
libpropono_la_SOURCES += propono-mdns.h propono-mdns.c
endif

libpropono_la_LDFLAGS = -export-dynamic -no-undefined -export-symbols-regex "^[[^_]].*"

libpropono_la_LIBADD = view/libview.la
# PROPONO_LIBS must be the last to ensure correct order on some platforms
libpropono_la_LIBADD += $(PROPONO_LIBS)


##############################
# The main binary
##############################

bin_PROGRAMS = propono

propono_SOURCES = \
  propono-main.c

propono_LDADD = \
	libpropono.la \
	$(PROPONO_LIBS)		\
	$(AVAHI_LIBS)		\
	$(GTKVNC_LIBS)		\
	$(NULL)

propono_LDFLAGS = -export-dynamic -no-undefined -export-symbols-regex "^[[^_]].*"

if APPLET

##############################
# MateComponent .server
##############################

serverdir =					\
	$(libdir)/matecomponent/servers
server_in_files =				\
	MATE_ProponoApplet.server.in
server_DATA =					\
	$(server_in_files:.server.in=.server)

$(server_in_files): $(server_in_files:.server.in=.server.in.in)
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" -e "s|\@VERSION\@|$(VERSION)|" $< > $@

@INTLTOOL_SERVER_RULE@

##############################
# The applet binary
##############################

libexec_PROGRAMS = propono-applet

propono_applet_CFLAGS =			\
	$(APPLET_CFLAGS)		\
	$(AVAHI_CFLAGS)			\
	$(NULL)

propono_applet_SOURCES =					\
	propono-applet.c					\
	propono-bookmarks.h propono-bookmarks.c			\
	propono-bookmarks-ui.h propono-bookmarks-ui.c			\
	propono-connection.h propono-connection.c		\
	propono-utils.h propono-utils.c				\
	propono-enums.h propono-enums.c				\
	propono-bookmarks-entry.h propono-bookmarks-entry.c	\
	propono-bookmarks-tree.h propono-bookmarks-tree.c	\
	propono-bookmarks-migration.h propono-bookmarks-migration.c \
	propono-plugins-engine.h propono-plugins-engine.c \
	propono-plugin.h propono-plugin.c \
	propono-plugin-loader.h propono-plugin-loader.c \
	propono-plugin-manager.h propono-plugin-manager.c \
	propono-plugin-dialog.h propono-plugin-dialog.c \
	propono-debug.h propono-debug.c \
	propono-plugin-info.h propono-plugin-info.c \
	propono-window.h propono-window.c \
	propono-commands.h propono-commands.c \
	propono-notebook.h propono-notebook.c \
	propono-tab.h propono-tab.c \
	propono-prefs.h propono-prefs.c \
	propono-app.h propono-app.c \
	propono-object-module.h propono-object-module.c \
	propono-dirs.h propono-dirs.c \
	propono-fav.h propono-fav.c \
	propono-connect.h propono-connect.c \
	propono-spinner.h propono-spinner.c \
	propono-marshal.h propono-marshal.c \
	pty_open.h pty_open.c \
	propono-ssh.h propono-ssh.c \
	propono-cache-prefs.h propono-cache-prefs.c \
	$(NULL)

if AVAHI
propono_applet_SOURCES += propono-mdns.h propono-mdns.c
endif

propono_applet_LDADD =			\
	view/libview.la			\
	$(APPLET_LIBS)			\
	$(PROPONO_LIBS)			\
	$(AVAHI_LIBS)			\
	$(NULL)
propono_applet_LDFLAGS = -export-dynamic -no-undefined -export-symbols-regex "^[[^_]].*"

endif

##############################
# Autogenerated stuff
##############################

propono_enum_headers = \
	$(top_srcdir)/propono/propono-connection.h \
	$(NULL)

propono-enums.c: @REBUILD@ $(propono_enum_headers)
	glib-mkenums    --fhead "#include <glib-object.h>\n" \
			--fhead "#include \"propono-enums.h\"\n\n" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--fprod "\n#include \"@filename@\"\n" \
			--vhead "static const G@Type@Value _@enum_name@_values[] = {" \
			--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "  { 0, NULL, NULL }\n};\n\n" \
			--vtail "GType\n@enum_name@_get_type (void)\n{\n" \
			--vtail "  static GType type = 0;\n\n" \
			--vtail "  if (!type)\n" \
			--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n\n" \
			--vtail "  return type;\n}\n\n" \
		$(propono_enum_headers) > $@

propono-enums.h: @REBUILD@ $(propono_enum_headers)
	glib-mkenums    --fhead "#ifndef __PROPONO_ENUMS_H__\n" \
			--fhead "#define __PROPONO_ENUMS_H__ 1\n\n" \
			--fhead "G_BEGIN_DECLS\n\n" \
			--ftail "G_END_DECLS\n\n" \
			--ftail "#endif /* __PROPONO_ENUMS_H__ */\n" \
			--fprod "\n/* --- @filename@ --- */" \
			--eprod "#define PROPONO_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
			--eprod "GType @enum_name@_get_type (void);\n" \
		$(propono_enum_headers) >  $@

propono-marshal.list: $(handwritten_sources) Makefile.am
	( cd $(srcdir) && \
	sed -n -e 's/.*propono_marshal_\([[:upper:][:digit:]]*__[[:upper:][:digit:]_]*\).*/\1/p' \
	$(handwritten_sources) ) \
	| sed -e 's/__/:/' -e 'y/_/,/' | sort -u > $@.tmp
	if cmp -s $@.tmp $@; then \
		rm $@.tmp; \
	else \
		mv $@.tmp $@; \
	fi

%-marshal.h: %-marshal.list Makefile
	glib-genmarshal --header --prefix=_$(subst -,_,$*)_marshal $< > $*-marshal.h

%-marshal.c: %-marshal.list Makefile
	echo "#include \"propono-marshal.h\"" > $@ && \
	glib-genmarshal --body --prefix=_$(subst -,_,$*)_marshal $< >> $*-marshal.c

BUILT_SOURCES = \
	propono-enums.c \
	propono-enums.h \
	propono-marshal.list \
	propono-marshal.c \
	propono-marshal.h \
	$(NULL)

##############################
# Telepathy stuff
##############################

if TELEPATHY

INCLUDES += $(TELEPATHY_CFLAGS)
propono_LDADD += $(TELEPATHY_LIBS)
handwritten_sources += \
	propono-tubes-manager.c propono-tubes-manager.h		\
	propono-tube-handler.c propono-tube-handler.h		\
	handler.c handler.h \
	$(NULL)

endif

DISTCLEANFILES =		\
	$(BUILT_SOURCES)	\
	$(server_in_files)	\
	$(server_DATA)		\
	$(NULL)

EXTRA_DIST =					\
	MATE_ProponoApplet.server.in.in	\
	propono-marshal.list \
	$(NULL)

dist-hook:
	cd $(distdir) ; rm -f $(DISTCLEANFILES)

$(propono_OBJECTS): $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
