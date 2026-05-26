## Process this file with automake to produce Makefile.in

if HAS_GNOME
if USING_GNOME2
GNOMECFLAGS = `pkg-config --cflags libgnomeui-2.0 libpanelapplet-2.0 libglade-2.0`
GNOMELINKAGE = `pkg-config --libs libgnomeui-2.0 libpanelapplet-2.0 libglade-2.0|sed -e 's/-lz //'`
else
GNOMECFLAGS = `gnome-config --cflags gnomeui applets libglade`
GNOMELINKAGE = `gnome-config --libs gnomeui applets libglade|sed -e 's/-lz //'`	
endif

EXTRA_PROGRAMS = gscope
endif
bin_PROGRAMS = cscope

## Install cscope-indexer script into bindir
dist_bin_SCRIPTS = cscope-indexer

if USING_LEX
LEXER_SOURCE=scanner.l
else
LEXER_SOURCE=fscanner.l
endif

cscope_SOURCES = $(LEXER_SOURCE) egrep.y alloc.c alloc.h basename.c build.c \
build.h command.c compath.c constants.h crossref.c dir.c display.c edit.c   \
exec.c find.c global.h gscope.c help.c history.c input.c invlib.c invlib.h  \
library.h logdir.c lookup.c lookup.h main.c mouse.c mygetenv.c mypopen.c    \
scanner.h version.h vp.h vpaccess.c vpfopen.c vpinit.c vpopen.c

EXTRA_cscope_SOURCES = scanner.l fscanner.l

LEXLIB = @LEXLIB@

cscope_LDADD =  $(CURSES_LIBS) $(LEXLIB) $(LIBOBJS)

AM_CPPFLAGS = $(CURSES_INCLUDEDIR)

## Hack to clean the generated files and not include them in a "make dist".
CLEANFILES = scanner.c fscanner.c egrep.c
#dist-hook:
#	rm -f $(distdir)/scanner.c $(distdir)/fscanner.c $(distdir)/egrep.c

EXTRA_DIST = emacs.e gmacs.ml

if HAS_GNOME
GNOME_SWITCH=-DUSING_GNOME_UI

## if we compile gscope under Linux, we need to supply libglade the callback pointers
if GNOME_LINUX
GNOME_LINUX_FLAG = "-rdynamic"
endif

gscope_CFLAGS = $(GNOMECFLAGS) $(GNOME_SWITCH)
gscope_LDADD =  $(CURSES_LIBS) $(LEXLIB)
gscope_SOURCES = gscope.c $(cscope_SOURCES)
gscope_LDFLAGS = $(GNOMELINKAGE) $(GNOME_LINUX_FLAG)
endif

## Post-install hook: create gscope and gscope-indexer symlinks if missing
install-exec-hook:
	@echo "Creating gscope symlink ..."; \
	cd "$(DESTDIR)$(bindir)" && \
	if [ ! -f gscope ] && [ ! -L gscope ]; then \
		ln -sf cscope gscope; \
		echo "  Created: $(DESTDIR)$(bindir)/gscope -> cscope"; \
	else \
		echo "  Exists:  $(DESTDIR)$(bindir)/gscope (skipped)"; \
	fi
	@echo "Creating gscope-indexer symlink ..."; \
	cd "$(DESTDIR)$(bindir)" && \
	if [ ! -f gscope-indexer ] && [ ! -L gscope-indexer ]; then \
		ln -sf cscope-indexer gscope-indexer; \
		echo "  Created: $(DESTDIR)$(bindir)/gscope-indexer -> cscope-indexer"; \
	else \
		echo "  Exists:  $(DESTDIR)$(bindir)/gscope-indexer (skipped)"; \
	fi

## Uninstall hook: remove gscope and gscope-indexer symlinks
uninstall-hook:
	@echo "Removing gscope symlink ..."; \
	if [ -L "$(DESTDIR)$(bindir)/gscope" ]; then \
		rm -f "$(DESTDIR)$(bindir)/gscope"; \
		echo "  Removed: $(DESTDIR)$(bindir)/gscope"; \
	fi
	@echo "Removing gscope-indexer symlink ..."; \
	if [ -L "$(DESTDIR)$(bindir)/gscope-indexer" ]; then \
		rm -f "$(DESTDIR)$(bindir)/gscope-indexer"; \
		echo "  Removed: $(DESTDIR)$(bindir)/gscope-indexer"; \
	fi
