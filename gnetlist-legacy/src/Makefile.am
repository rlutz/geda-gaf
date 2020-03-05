## Process this file with automake to produce Makefile.in

bin_PROGRAMS = gnetlist-legacy

# don't forget all *.h files */
gnetlist_legacy_SOURCES = \
	g_netlist.c \
	g_rc.c \
	g_register.c \
	globals.c \
	gnetlist.c \
	i_vars.c \
	parsecmd.c \
	s_cpinlist.c \
	s_hierarchy.c \
	s_misc.c \
	s_net.c \
	s_netattrib.c \
	s_netlist.c \
	s_rename.c \
	s_traverse.c \
	vams_misc.c

gnetlist_legacy_CPPFLAGS = -I$(top_srcdir)/libgeda/include \
	-I$(srcdir)/../include -I$(top_srcdir) -I$(includedir)
gnetlist_legacy_CFLAGS = $(GCC_CFLAGS) $(MINGW_CFLAGS) $(GLIB_CFLAGS) \
	$(GIO_CFLAGS) $(GUILE_CFLAGS) $(GDK_PIXBUF_CFLAGS)
gnetlist_legacy_LDFLAGS = $(GLIB_LIBS) $(GIO_LIBS) $(GUILE_LIBS) \
	$(GDK_PIXBUF_LIBS)
gnetlist_legacy_LDADD = $(top_builddir)/libgeda/src/libgeda.la

localedir = @datadir@/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

MOSTLYCLEANFILES = *.log *.ps core FILE *~
CLEANFILES = *.log *.ps core FILE *~
DISTCLEANFILES = *.log core FILE *~
MAINTAINERCLEANFILES = *.log *.ps core FILE *~ Makefile.in
