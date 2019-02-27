GNETLIST = $(bindir)/gnetlist
GAF = $(bindir)/gaf

sources = \
	gafrc \
	gnetlistrc \
	gTAG.sch \
	gTAG-consio.sch \
	gTAG-consio.sym \
	gTAG-jtagio.sch \
	gTAG-jtagio.sym \
	gTAG-psu.sch \
	gTAG-psu.sym \
	gTAG-ucont.sch \
	gTAG-ucont.sym \
	7414-1.sym \
	copyleft.sym \
	cy7c64603-52nc.sym \
	max882.sym \
	sn75240pw.sym

targets = \
	gTAG.bom \
	gTAG-pcb.net \
	gTAG.ps \
	gTAG-consio.ps \
	gTAG-jtagio.ps \
	gTAG-psu.ps \
	gTAG-ucont.ps

extra = \
	README \
	ChangeLog-1.0 \
	attribs \
	crdist.sh

gTAG-pcb.net: $(sources)
	$(AM_V_GEN)if [ -x $(GNETLIST) ]; then \
	  cd $(srcdir) && $(GNETLIST) -g PCB -o $@ gTAG.sch; \
	else \
	  echo '*** Skipping, re-run "make" after "make install" ***'; \
	fi

gTAG.bom: $(sources)
	$(AM_V_GEN)if [ -x $(GNETLIST) ]; then \
	  cd $(srcdir) && $(GNETLIST) -g bom -o $@ gTAG.sch; \
	else \
	  echo '*** Skipping, re-run "make" after "make install" ***'; \
	fi

.sch.ps:
	$(AM_V_GEN)if [ -x $(GAF) ]; then \
	  cd $(srcdir) && ( \
	    $(GAF) export -p iso_a4 -m "1cm;1cm;1cm;1cm" -o $@ $< && \
	    sed -e '/^%%CreationDate: /d' -i $@); \
	else \
	  echo '*** Skipping, re-run "make" after "make install" ***'; \
	fi

exampledir = $(docdir)/examples/gTAG/
example_DATA = $(sources) $(targets) $(extra)

EXTRA_DIST = $(example_DATA)

MAINTAINERCLEANFILES = $(targets)
