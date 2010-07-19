## Process this file with automake to produce Makefile.in

## Created by Anjuta

gladedir = $(datadir)/sphone/glade
glade_DATA = sphone.glade

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(SPHONE_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = sphone

sphone_SOURCES = \
	main.c \
	ofono.c \
	ofono.h \
	sphone-call.h \
	sphone-call.c \
	sphone-manager.h \
	sphone-manager.c \
	notification.c \
	notification.h \
	dbus-marshalers.c \
	dbus-marshalers.h \
	gui-calls-manager.c \
	gui-calls-manager.h \
	keypad.c \
	keypad.h \
	gui-dialer.c \
	gui-dialer.h \
	utils.c \
	utils.h \
	gui-sms.c \
	gui-sms.h \
	keys-grab.c \
	keys-grab.h \
	store.c \
	store.h \
	sphone-store-tree-model.h \
	sphone-store-tree-model.c \
	gui-contact-view.c \
	gui-contact-view.h \
	book-import.c \
	book-import.h \
	proximity.c \
	proximity.h

sphone_LDFLAGS = \
	-Wl,--export-dynamic

sphone_LDADD = $(SPHONE_LIBS)

EXTRA_DIST = $(glade_DATA)

# Remove glade directory on uninstall
uninstall-local:
	-rm -r $(gladedir)
	-rm -r $(datadir)/sphone
