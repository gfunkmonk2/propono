SUBDIRS = icons

man_MANS = propono.1

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = propono-@PROPONO_API_VERSION@.pc

propono-@PROPONO_API_VERSION@.pc: propono.pc
	cp propono.pc propono-@PROPONO_API_VERSION@.pc

uidir = $(datadir)/propono
ui_DATA = propono-ui.xml MATE_ProponoApplet.xml propono.ui propono-fav-ui.xml

@INTLTOOL_DESKTOP_RULE@
desktopdir       = $(datadir)/applications
desktop_in_files = propono.desktop.in propono-file.desktop.in
desktop_DATA     = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_XML_RULE@
MIMEFILES = propono-mime.xml.in
mimedir = $(datadir)/mime/packages
mime_DATA = propono-mime.xml

schemasdir	 = $(MATECONF_SCHEMA_FILE_DIR)
schemas_in_files = propono.schemas.in
schemas_DATA 	 = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

if MATECONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schemas_DATA) ; do \
			MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(top_builddir)/data/$$p ; \
		done \
	fi
else
install-data-local:
endif

CLEANFILES = \
	$(desktop_DATA) \
	$(mime_DATA) \
	$(schemas_DATA) \
	$(pkgconfig_DATA) \
	org.freedesktop.Telepathy.Client.Propono.service

EXTRA_DIST = \
	$(glade_DATA) \
	$(man_MANS) \
	$(ui_DATA) \
	$(desktop_in_files) \
	$(MIMEFILES) \
	$(schemas_in_files) \
	propono.pc.in \
	propono.schemas.in.in \
	Propono.client \
	org.freedesktop.Telepathy.Client.Propono.service.in

if TELEPATHY

clientfiledir = $(datarootdir)/telepathy/clients
clientfile_DATA = Propono.client

servicedir = $(datadir)/dbus-1/services
service_DATA = org.freedesktop.Telepathy.Client.Propono.service

org.freedesktop.Telepathy.Client.Propono.service: org.freedesktop.Telepathy.Client.Propono.service.in \
		Makefile
	@sed -e "s|[@]bindir[@]|$(bindir)|" $< > $@

endif

UPDATE_MIME = \
        if [ -f $(DESTDIR)$(datadir)/mime/packages/freedesktop.org.xml ] ; then \
                if which update-mime-database>/dev/null 2>&1; then \
                        update-mime-database $(DESTDIR)$(datadir)/mime; \
                fi \
        fi

UPDATE_DESKTOP = \
        if [ -f $(DESTDIR)$(datadir)/applications/defaults.list ] ; then \
                if which update-desktop-database>/dev/null 2>&1 ; then \
                        update-desktop-database; \
                fi \
        fi

install-data-hook: 
	$(UPDATE_MIME)
	$(UPDATE_DESKTOP)

uninstall-hook: 
	$(UPDATE_MIME)
	$(UPDATE_DESKTOP)

-include $(top_srcdir)/git.mk
