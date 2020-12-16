symbol_dirs = \
	4000 \
	74 \
	IEC417 \
	allegro \
	altera \
	amphenol \
	analog \
	apex \
	asic \
	asicpads \
	bus \
	cascade \
	connector \
	dec \
	diode \
	ecl \
	idt \
	io \
	irf \
	lattice \
	linear \
	local \
	maxim \
	memory \
	micro \
	minicircuits \
	misc \
	national \
	net-power \
	opto \
	philips \
	pla \
	power \
	radio \
	relay \
	rf \
	spice \
	st \
	supervisor \
	switcap \
	switch \
	titleblock \
	transistor \
	tube \
	xilinx

extra_symbol_dirs = \
	gnetman \
	verilog \
	vhdl

SUBDIRS = documentation

gafrcddir = $(GEDADATADIR)/gafrc.d
dist_gafrcd_DATA = geda-clib.scm

datasymdir = $(GEDADATADIR)/sym
dataextrasymdir = $(GEDADATADIR)/extra-sym

EXTRA_DIST = ChangeLog ChangeLog-1.0 \
	radio/README AUTHORS

if HAVE_GIT_REPO
ChangeLog: $(top_builddir)/stamp-git
	( \
	  cd $(srcdir) && \
	  $(GIT) log --pretty=medium $(CHANGELOG_BASE).. -- . || \
	  echo "WARNING: ChangeLog information not available from git" >&2 ; \
	) > $@
endif HAVE_GIT_REPO

# Copies symbols into distribution
dist-hook:
	@for lib in $(symbol_dirs) $(extra_symbol_dirs); do \
	  $(MKDIR_P) $(distdir)/$$lib && \
	  list=`(cd $(srcdir)/$$lib && find . -name '*.[sS][yY][mM]' | sed -e 's:^\./::')`; \
	  for file in $$list; do \
	    cp -p $(srcdir)/$$lib/$$file $(distdir)/$$lib || exit $$?; \
	  done; \
	done;

# Installs symbols
install-data-local:
	@$(NORMAL_INSTALL)
	@for lib in $(symbol_dirs); do \
	  d="$(DESTDIR)$(datasymdir)/$$lib"; \
	  $(MKDIR_P) "$$d"; \
	  list=`(cd $(srcdir)/$$lib && find . -name '*.[sS][yY][mM]' | sed -e 's:^\./::')`; \
	  if test -n "$$list"; then \
	    echo " ( cd $(srcdir)/$$lib && $(INSTALL_DATA)" $$list "'$$d' )"; \
	    (cd $(srcdir)/$$lib && $(INSTALL_DATA) $$list "$$d") || exit $$?; \
	  fi; \
	done
	@for lib in $(extra_symbol_dirs); do \
	  d="$(DESTDIR)$(dataextrasymdir)/$$lib"; \
	  $(MKDIR_P) "$$d"; \
	  list=`(cd $(srcdir)/$$lib && find . -name '*.[sS][yY][mM]' | sed -e 's:^\./::')`; \
	  if test -n "$$list"; then \
	    echo " ( cd $(srcdir)/$$lib && $(INSTALL_DATA)" $$list "'$$d' )"; \
	    (cd $(srcdir)/$$lib && $(INSTALL_DATA) $$list "$$d") || exit $$?; \
	    echo " ( cd '$(DESTDIR)$(datasymdir)/$$lib' && rm -f" $$list ")"; \
	    (cd "$(DESTDIR)$(datasymdir)/$$lib" && rm -f $$list || true); \
	  fi; \
	  rmdir "$(DESTDIR)$(datasymdir)/$$lib" || true; \
	done

# Uninstalls symbols
uninstall-local:
	@$(NORMAL_UNINSTALL)
	@for lib in $(symbol_dirs); do \
	  d="$(DESTDIR)$(datasymdir)/$$lib"; \
	  list=`(cd $(srcdir)/$$lib && find . -name '*.[sS][yY][mM]' | sed -e 's:^\./::')`; \
	  if test -n "$$list"; then \
	    echo " ( cd '$$d' && rm -f" $$list ")"; \
	    (cd "$$d" && rm -f $$list) || exit $$?; \
	  fi; \
	done
	@for lib in $(extra_symbol_dirs); do \
	  d="$(DESTDIR)$(dataextrasymdir)/$$lib"; \
	  list=`(cd $(srcdir)/$$lib && find . -name '*.[sS][yY][mM]' | sed -e 's:^\./::')`; \
	  if test -n "$$list"; then \
	    echo " ( cd '$$d' && rm -f" $$list ")"; \
	    (cd "$$d" && rm -f $$list) || exit $$?; \
	  fi; \
	done

MOSTLYCLEANFILES = *.log *~
CLEANFILES = *.log *~
DISTCLEANFILES = *.log core FILE *~
MAINTAINERCLEANFILES = *.log *~ Makefile.in ChangeLog
