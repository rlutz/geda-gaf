# Copyright (C) 2013-2020 Roland Lutz
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

noinst_SCRIPTS = xorn gnetlist
CLEANFILES = xorn xornT gnetlist gnetlistT
EXTRA_DIST = xorn.in gnetlist.in

xorn: $(srcdir)/xorn.in Makefile
	$(AM_V_GEN)sed \
	    -e 's,[@]pyexecdir[@],$(abs_top_builddir)/built-packages,g' \
	    -e 's,[@]commanddir[@],$(abs_srcdir),g' \
		$(srcdir)/$@.in > $@; \
	chmod +x $@

xornT: $(srcdir)/xorn.in Makefile
	$(AM_V_GEN)sed \
	    -e 's,[@]pyexecdir[@],$(pyexecdir),g' \
	    -e 's,[@]commanddir[@],$(commanddir),g' \
		$(srcdir)/xorn.in > $@

gnetlist: $(srcdir)/gnetlist.in Makefile
	$(AM_V_GEN)sed \
	    -e 's,[@]pyexecdir[@],$(abs_top_builddir)/built-packages,g' \
	    -e 's,[@]XORN[@],$(abs_builddir)/xorn,g' \
	    -e 's,[@]gedadatadir[@],$(gedadatadir),g' \
	    -e 's,[@]gedasysconfdir[@],$(gedasysconfdir),g' \
	    -e 's,[@]backenddir[@],$(abs_top_srcdir)/src/backend,g' \
		$(srcdir)/$@.in > $@; \
	chmod +x $@

gnetlistT: $(srcdir)/gnetlist.in Makefile
	$(AM_V_GEN)sed \
	    -e 's,[@]pyexecdir[@],$(pyexecdir),g' \
	    -e 's,[@]XORN[@],$(bindir)/xorn,g' \
	    -e 's,[@]gedadatadir[@],$(gedadatadir),g' \
	    -e 's,[@]gedasysconfdir[@],$(gedasysconfdir),g' \
	    -e 's,[@]backenddir[@],$(pkglibdir)/backends,g' \
		$(srcdir)/gnetlist.in > $@

install-exec-local: xornT gnetlistT
	@$(NORMAL_INSTALL)
	@test -n "$(bindir)" || exit 0; \
	echo " $(MKDIR_P) '$(DESTDIR)$(bindir)'"; \
	$(MKDIR_P) "$(DESTDIR)$(bindir)" || exit 1; \
	name=`echo "xorn" | sed -e 's,.*/,,;$(transform)'`; \
	echo " $(INSTALL_SCRIPT) xornT '$(DESTDIR)$(bindir)/$$name'"; \
	$(INSTALL_SCRIPT) xornT "$(DESTDIR)$(bindir)/$$name" || exit $$?; \
	name=`echo "gnetlist" | sed -e 's,.*/,,;$(transform)'`; \
	echo " $(INSTALL_SCRIPT) gnetlistT '$(DESTDIR)$(bindir)/$$name'"; \
	$(INSTALL_SCRIPT) gnetlistT "$(DESTDIR)$(bindir)/$$name" || exit $$?

uninstall-local:
	@$(NORMAL_UNINSTALL)
	@test -n "$(bindir)" || exit 0; \
	files=`echo "xorn" "gnetlist" | sed -e 's,.*/,,;$(transform)'`; \
	dir='$(DESTDIR)$(bindir)'; \
	$(am__uninstall_files_from_dir)

commanddir = $(pkglibdir)
command_PYTHON = \
	convert.py \
	extract.py \
	find-symbol-by-pinout.py \
	netlist.py
