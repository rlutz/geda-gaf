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

maint_targets = \
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

maint_scripts = \
	regen-netlists.sh \
	regen-postscript.sh

exampledir = $(docdir)/examples/gTAG/
example_DATA = $(sources) $(maint_targets) $(extra)

EXTRA_DIST = $(example_DATA) $(maint_scripts)

MAINTAINERCLEANFILES = $(maint_targets)
