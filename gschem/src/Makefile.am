bin_PROGRAMS = gschem

BUILT_SOURCES = \
	actions.decl.x \
	actions.init.x \
	g_attrib.x \
	g_hook.x \
	g_keys.x \
	g_util.x \
	g_window.x \
	g_select.x

gschem_SOURCES = \
	a_zoom.c \
	g_action.c \
	g_attrib.c \
	g_funcs.c \
	g_hook.c \
	g_keys.c \
	g_rc.c \
	g_register.c \
	g_select.c \
	g_util.c \
	g_window.c \
	globals.c \
	gschem.c \
	gschemhotkeystore.c \
	gschem_about_dialog.c \
	gschem_accel_label.c \
	gschem_action.c \
	gschem_alignment_combo.c \
	gschem_arc_dialog.c \
	gschem_binding.c \
	gschem_binding_integer.c \
	gschem_bottom_widget.c \
	gschem_change_notification.c \
	gschem_close_confirmation_dialog.c \
	gschem_compselect_dockable.c \
	gschem_dialog.c \
	gschem_dialog_misc.c \
	gschem_dockable.c \
	gschem_fill_swatch_cell_renderer.c \
	gschem_find_text_dockable.c \
	gschem_find_text_widget.c \
	gschem_hotkey_dialog.c \
	gschem_integer_combo_box.c \
	gschem_log_dockable.c \
	gschem_macro_widget.c \
	gschem_main_window.c \
	gschem_messages_dockable.c \
	gschem_multiattrib_dockable.c \
	gschem_object_properties_dockable.c \
	gschem_options.c \
	gschem_options_dockable.c \
	gschem_page_geometry.c \
	gschem_page_view.c \
	gschem_pagesel_dockable.c \
	gschem_patch.c \
	gschem_patch_dockable.c \
	gschem_pin_type_combo.c \
	gschem_preview.c \
	gschem_rotation_combo.c \
	gschem_selection_adapter.c \
	gschem_show_hide_text_widget.c \
	gschem_slot_edit_dialog.c \
	gschem_swatch_column_renderer.c \
	gschem_text_properties_dockable.c \
	gschem_toplevel.c \
	i_basic.c \
	i_vars.c \
	m_basic.c \
	o_arc.c \
	o_attrib.c \
	o_basic.c \
	o_box.c \
	o_buffer.c \
	o_bus.c \
	o_circle.c \
	o_complex.c \
	o_copy.c \
	o_delete.c \
	o_find.c \
	o_grips.c \
	o_line.c \
	o_misc.c \
	o_move.c \
	o_net.c \
	o_ognrst.c \
	o_path.c \
	o_picture.c \
	o_pin.c \
	o_place.c \
	o_select.c \
	o_slot.c \
	o_text.c \
	o_undo.c \
	parsecmd.c \
	s_stretch.c \
	stroke.c \
	x_attribedit.c \
	x_autonumber.c \
	x_basic.c \
	x_clipboard.c \
	x_color.c \
	x_colorcb.c \
	x_controlfd.c \
	x_dialog.c \
	x_event.c \
	x_fam.c \
	x_fileselect.c \
	x_fstylecb.c \
	x_grid.c \
	x_grid_size_sb.c \
	x_hierarchy.c \
	x_highlevel.c \
	x_image.c \
	x_integerls.c \
	x_linecapcb.c \
	x_linetypecb.c \
	x_lowlevel.c \
	x_menus.c \
	x_misc.c \
	x_newtext.c \
	x_print.c \
	x_rc.c \
	x_script.c \
	x_stroke.c \
	x_window.c

gschem_CPPFLAGS = -I$(top_srcdir)/libgeda/include  -I$(srcdir)/../include \
	-I$(top_srcdir) -I$(includedir)
gschem_CFLAGS = $(GCC_CFLAGS) $(LIBFAM_CFLAGS) \
	$(MINGW_CFLAGS) \
	$(GLIB_CFLAGS) $(GIO_CFLAGS) $(GTK_CFLAGS) $(GTHREAD_CFLAGS) \
	$(GUILE_CFLAGS)
gschem_LDFLAGS = $(LIBFAM_LDFLAGS) $(GLIB_LIBS) $(GIO_LIBS) $(GTK_LIBS) \
	$(GTHREAD_LIBS) $(GUILE_LIBS) $(MINGW_GUI_LDFLAGS)
gschem_LDADD = \
	$(top_builddir)/libgedacairo/libgedacairo.la \
	$(top_builddir)/libgeda/src/libgeda.la


# This is used to generate boilerplate for defining Scheme functions
# in C.
SUFFIXES = .x
snarf_cpp_opts = $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(gschem_CPPFLAGS) $(AM_CFLAGS) $(gschem_CFLAGS) $(CPPFLAGS)
AM_V_SNARF = $(am__v_SNARF_@AM_V@)
am__v_SNARF_ = $(am__v_SNARF_@AM_DEFAULT_V@)
am__v_SNARF_0 = @echo "  SNARF   " $@;
am__v_SNARF_1 =
.c.x:
	$(AM_V_SNARF)CPP="$(CPP)" $(GUILE_SNARF) -o $@ $< $(snarf_cpp_opts)

actions.decl.x: actions.decl.h actions.c Makefile
	$(AM_V_GEN)$(CPP) -I$(srcdir) -c $(srcdir)/actions.decl.h | \
	  sed -e '/^.*KEEP_LINE /!d' -e 's/^.*KEEP_LINE //' > $@.tmp && \
	mv $@.tmp $@

actions.init.x: actions.init.c actions.c Makefile
	$(AM_V_GEN)$(CPP) -I$(srcdir) -c $(srcdir)/actions.init.c | \
	  sed -e '/^.*KEEP_LINE /!d' -e 's/^.*KEEP_LINE //' > $@.tmp && \
	mv $@.tmp $@

EXTRA_DIST = actions.c actions.decl.h actions.init.c

localedir = @datadir@/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

MOSTLYCLEANFILES = *.log core FILE *~
CLEANFILES = *.log core FILE *~ $(BUILT_SOURCES)
DISTCLEANFILES = *.log core FILE *~
MAINTAINERCLEANFILES = *.log core FILE *~ Makefile.in
