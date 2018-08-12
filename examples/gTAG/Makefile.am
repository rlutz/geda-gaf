GNETLIST = $(abs_top_builddir)/xorn/src/command/gnetlist
GAF = $(abs_top_builddir)/gaf/gaf

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
	cd $(srcdir) && $(GNETLIST) -g PCB -o $@ gTAG.sch

gTAG.bom: $(sources)
	cd $(srcdir) && $(GNETLIST) -g bom -o $@ gTAG.sch

.sch.ps:
	cd $(srcdir) && ( \
	    $(GAF) export -p iso_a4 -m "1cm;1cm;1cm;1cm" -o $@ $< && \
	    sed -e '/^%%CreationDate: /d' -i $@)

exampledir = $(docdir)/examples/gTAG/
example_DATA = $(sources) $(targets) $(extra)

EXTRA_DIST = $(example_DATA)

MAINTAINERCLEANFILES = $(targets)
