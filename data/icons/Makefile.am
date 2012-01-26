NULL =

public_icons_themes = \
	hicolor	\
	$(NULL)

public_icons = \
	hicolor_apps_16x16_propono.png \
	hicolor_apps_22x22_propono.png \
	hicolor_apps_24x24_propono.png \
	hicolor_apps_32x32_propono.png \
	hicolor_apps_48x48_propono.png \
	hicolor_apps_scalable_propono.svg \
	hicolor_mimetypes_16x16_application-mate-remote-connection.png \
	hicolor_mimetypes_16x16_application-mate-vnc.png \
	hicolor_mimetypes_22x22_application-mate-remote-connection.png \
	hicolor_mimetypes_22x22_application-mate-vnc.png \
	hicolor_mimetypes_24x24_application-mate-remote-connection.png \
	hicolor_mimetypes_24x24_application-mate-vnc.png \
	hicolor_mimetypes_32x32_application-mate-remote-connection.png \
	hicolor_mimetypes_32x32_application-mate-vnc.png \
	hicolor_mimetypes_48x48_application-mate-remote-connection.png \
	hicolor_mimetypes_48x48_application-mate-vnc.png \
	hicolor_mimetypes_scalable_application-mate-remote-connection.svg \
	hicolor_mimetypes_scalable_application-mate-vnc.svg \
	hicolor_status_16x16_view-minimize.png \
	hicolor_status_22x22_view-minimize.png \
	hicolor_status_32x32_view-minimize.png \
	hicolor_status_48x48_view-minimize.png \
	$(NULL)

private_icons = \
	$(NULL)

noinst_DATA = \
	hicolor_apps_16x16_propono.svg \
	hicolor_apps_16x16_propono.xcf \
	hicolor_apps_22x22_propono.svg \
	hicolor_apps_32x32_propono.svg \
	$(NULL)

EXTRA_DIST = \
	$(public_icons)		\
	$(private_icons)	\
	$(noinst_DATA)		\
	$(NULL)

###############################################################################

gtk_update_icon_cache = gtk-update-icon-cache -f -t

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		for theme in $(public_icons_themes); do \
			$(gtk_update_icon_cache) $(datadir)/icons/$$theme; \
		done; \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		for theme in $(public_icons_themes); do \
			echo "***   $(gtk_update_icon_cache) $(datadir)/icons/$$theme"; \
		done; \
	fi

install-icons:
	for icon in $(public_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		mkdir -p $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done; \
	for icon in $(private_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		mkdir -p $(DESTDIR)$(pkgdatadir)/icons/$$THEME/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(pkgdatadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done

uninstall-icons:
	-for icon in $(public_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		rm -f $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done; \
	for icon in $(private_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		rm -f $(DESTDIR)$(pkgdatadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done

install-data-local: install-icons update-icon-cache

uninstall-local: uninstall-icons update-icon-cache

-include $(top_srcdir)/git.mk
