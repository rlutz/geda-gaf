
rcdatadir = $(GEDARCDIR)

dist_rcdata_DATA = \
	gschem-colormap-darkbg \
	gschem-colormap-whitebg \
	gschem-colormap-whitebg-bw \
	gschem-colormap-lightbg \
	gschem-colormap-bw \
	gschem-gtkrc

rcdata_DATA = system-gschemrc
EXTRA_DIST = system-gschemrc.scm

system-gschemrc: $(top_builddir)/configure system-gschemrc.scm
	@date_ver=$(DATE_VERSION); \
	sed -e"s:[@]DATE_VERSION@:$$date_ver:" \
	  < $(srcdir)/system-gschemrc.scm > system-gschemrc.new; \
	if diff system-gschemrc system-gschemrc.new > /dev/null 2>&1; then \
	  rm system-gschemrc.new; \
	  echo "system-gschemrc is unchanged"; \
	else \
	  echo "Recreating system-gschemrc"; \
	  mv system-gschemrc.new system-gschemrc; \
	fi

MOSTLYCLEANFILES = *.log *~
CLEANFILES = *.log *~
DISTCLEANFILES = *.log core FILE *~ system-gschemrc
MAINTAINERCLEANFILES = system-gschemrc system-gnetlistrc system-gsymbolrc *.log *~ Makefile.in
