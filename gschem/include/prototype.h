/* $Id$ */

/* a_zoom.c */
void a_zoom(GschemToplevel *w_current, GschemPageView *page_view, int dir, int selected_from);
void a_zoom_box_start(GschemToplevel *w_current, int x, int y);
void a_zoom_box_end(GschemToplevel *w_current, int x, int y);
void a_zoom_box_motion(GschemToplevel *w_current, int x, int y);
void a_zoom_box_invalidate_rubber(GschemToplevel *w_current);
void a_zoom_box_draw_rubber(GschemToplevel *w_current, EdaRenderer *renderer);
/* g_action.c */
gboolean g_action_get_position (gboolean snap, int *x, int *y);
/* g_attrib.c */
void g_init_attrib ();
/* g_funcs.c */
SCM g_funcs_pdf(SCM filename);
SCM g_funcs_image(SCM filename);
SCM g_funcs_exit(void);
SCM g_funcs_log(SCM msg);
SCM g_funcs_msg(SCM msg);
SCM g_funcs_confirm(SCM msg);
SCM g_funcs_filesel(SCM msg, SCM templ, SCM flags);
SCM g_funcs_use_rc_values(void);
/* g_hook.c */
void g_init_hook ();
void g_run_hook_object (GschemToplevel *w_current, const char *name, OBJECT *obj);
void g_run_hook_object_list (GschemToplevel *w_current, const char *name, GList *obj_lst);
void g_run_hook_page (GschemToplevel *w_current, const char *name, PAGE *page);
EdascmHookProxy *g_hook_new_proxy_by_name (const char *name);
/* g_keys.c */
void g_keys_reset (GschemToplevel *w_current);
int g_keys_execute(GschemToplevel *w_current, GdkEventKey *event);
void g_init_keys ();
/* g_rc.c */
void g_rc_parse_gtkrc();
SCM g_rc_gschem_version(SCM version);
SCM g_rc_net_direction_mode(SCM mode);
SCM g_rc_net_selection_mode(SCM mode);
SCM g_rc_action_feedback_mode(SCM mode);
SCM g_rc_zoom_with_pan(SCM mode);
SCM g_rc_logging(SCM mode);
SCM g_rc_embed_components(SCM mode);
SCM g_rc_text_size(SCM size);
SCM g_rc_text_caps_style(SCM mode);
SCM g_rc_snap_size(SCM size);
SCM g_rc_logging_destination(SCM mode);
SCM g_rc_attribute_name(SCM path);
SCM g_rc_scrollbars(SCM mode);
SCM g_rc_image_color(SCM mode);
SCM g_rc_image_size(SCM width, SCM height);
SCM g_rc_log_window(SCM mode);
SCM g_rc_log_window_type(SCM mode);
SCM g_rc_third_button(SCM mode);
SCM g_rc_third_button_cancel(SCM mode);
SCM g_rc_middle_button(SCM mode);
SCM g_rc_scroll_wheel(SCM mode);
SCM g_rc_net_consolidate(SCM mode);
SCM g_rc_file_preview(SCM mode);
SCM g_rc_enforce_hierarchy(SCM mode);
SCM g_rc_fast_mousepan(SCM mode);
SCM g_rc_raise_dialog_boxes_on_expose(SCM mode);
SCM g_rc_continue_component_place(SCM mode);
SCM g_rc_undo_levels(SCM levels);
SCM g_rc_undo_control(SCM mode);
SCM g_rc_undo_type(SCM mode);
SCM g_rc_undo_panzoom(SCM mode);
SCM g_rc_draw_grips(SCM mode);
SCM g_rc_netconn_rubberband(SCM mode);
SCM g_rc_magnetic_net_mode(SCM mode);
SCM g_rc_window_size(SCM width, SCM height);
SCM g_rc_warp_cursor(SCM mode);
SCM g_rc_toolbars(SCM mode);
SCM g_rc_handleboxes(SCM mode);
SCM g_rc_bus_ripper_size(SCM size);
SCM g_rc_bus_ripper_type(SCM mode);
SCM g_rc_bus_ripper_rotation(SCM mode);
SCM g_rc_force_boundingbox(SCM mode);
SCM g_rc_grid_mode(SCM mode);
SCM g_rc_dots_grid_dot_size(SCM dotsize);
SCM g_rc_dots_grid_mode(SCM mode);
SCM g_rc_dots_grid_fixed_threshold(SCM spacing);
SCM g_rc_mesh_grid_display_threshold(SCM spacing);
SCM g_rc_add_attribute_offset(SCM offset);
SCM g_rc_auto_save_interval(SCM seconds);
SCM g_rc_mousepan_gain(SCM mode);
SCM g_rc_keyboardpan_gain(SCM mode);
SCM g_rc_select_slack_pixels(SCM pixels);
SCM g_rc_zoom_gain(SCM gain);
SCM g_rc_scrollpan_steps(SCM steps);
SCM g_rc_display_color_map (SCM scm_map);
SCM g_rc_display_outline_color_map (SCM scm_map);
/* g_register.c */
void g_register_funcs(void);
/* g_select.c */
void g_init_select ();
/* g_util.c */
void g_init_util ();
/* g_window.c */
GschemToplevel *g_current_window ();
void g_dynwind_window (GschemToplevel *w_current);
void g_init_window ();
/* globals.c */
/* gschem.c */
typedef void (*gschem_atexit_func)(gpointer data);
void gschem_atexit(gschem_atexit_func func, gpointer data);
void main_prog(void *closure, int argc, char *argv[]);
int main(int argc, char *argv[]);
/* i_basic.c */
void i_action_start(GschemToplevel *w_current);
void i_action_stop(GschemToplevel *w_current);
void i_action_update_status(GschemToplevel *w_current,gboolean inside_action);
void i_show_state(GschemToplevel *w_current, const char *message);
void i_set_state(GschemToplevel *w_current, enum x_states newstate);
void i_set_state_msg(GschemToplevel *w_current, enum x_states newstate, const char *message);
void i_update_middle_button(GschemToplevel *w_current, GschemAction *action, const char *string);
void i_update_toolbar(GschemToplevel *w_current);
void i_update_menus(GschemToplevel *w_current);
void i_update_filename(GschemToplevel *w_current);
void i_update_grid_info(GschemToplevel *w_current);
void i_update_grid_info_callback (GschemPageView *view, GschemToplevel *w_current);
void i_cancel(GschemToplevel *w_current);
void i_buffer_copy(GschemToplevel *w_current, int n, GschemAction *action);
void i_buffer_cut(GschemToplevel *w_current, int n, GschemAction *action);
void i_buffer_paste(GschemToplevel *w_current, int n, GschemAction *action);
/* i_vars.c */
void i_vars_set(GschemToplevel *w_current);
void i_vars_freenames();
void i_vars_init_gschem_defaults (void);
void i_vars_atexit_save_user_config (gpointer user_data);
 /* m_basic.c */
int snap_grid(GschemToplevel *w_current, int input);
int clip_nochange(GschemPageGeometry *geometry, int x1, int y1, int x2, int y2);
int visible(GschemToplevel *w_current, int wleft, int wtop, int wright, int wbottom);
double round_5_2_1(double unrounded);
/* o_arc.c */
void o_arc_invalidate_rubber(GschemToplevel *w_current);
void o_arc_start(GschemToplevel *w_current, int x, int y);
void o_arc_end1(GschemToplevel *w_current, int x, int y);
void o_arc_end4(GschemToplevel *w_current, int radius, int start_angle, int sweep_angle);
void o_arc_motion(GschemToplevel *w_current, int x, int y, int whichone);
void o_arc_draw_rubber(GschemToplevel *w_current, EdaRenderer *renderer);
/* o_attrib.c */
void o_attrib_add_selected(GschemToplevel *w_current, SELECTION *selection, OBJECT *selected);
void o_attrib_deselect_invisible(GschemToplevel *w_current, SELECTION *selection, OBJECT *selected);
void o_attrib_select_invisible(GschemToplevel *w_current, SELECTION *selection, OBJECT *selected);
void o_attrib_toggle_visibility(GschemToplevel *w_current, OBJECT *object);
void o_attrib_toggle_show_name_value(GschemToplevel *w_current, OBJECT *object, int new_show_name_value);
OBJECT *o_attrib_add_attrib(GschemToplevel *w_current, const char *text_string, int visibility, int show_name_value, OBJECT *object);
/* o_basic.c */
void o_redraw_rect (GschemToplevel *w_current, GdkDrawable *drawable, PAGE *page, GschemPageGeometry *geometry, GdkRectangle *rectangle);
int o_invalidate_rubber(GschemToplevel *w_current);
int o_redraw_cleanstates(GschemToplevel *w_current);
void o_invalidate_rect(GschemToplevel *w_current, int x1, int y1, int x2, int y2);
void o_invalidate(GschemToplevel *w_current, OBJECT *object);
void o_invalidate_glist(GschemToplevel *w_current, GList *list);
/* o_box.c */
void o_box_invalidate_rubber(GschemToplevel *w_current);
void o_box_start(GschemToplevel *w_current, int x, int y);
void o_box_end(GschemToplevel *w_current, int x, int y);
void o_box_motion(GschemToplevel *w_current, int x, int y);
void o_box_draw_rubber (GschemToplevel *w_current, EdaRenderer *renderer);
/* o_buffer.c */
void o_buffer_copy(GschemToplevel *w_current, int buf_num);
void o_buffer_cut(GschemToplevel *w_current, int buf_num);
int o_buffer_paste_start(GschemToplevel *w_current, int x, int y, int buf_num);
void o_buffer_init(void);
void o_buffer_free(GschemToplevel *w_current);
/* o_bus.c */
void o_bus_start(GschemToplevel *w_current, int x, int y);
void o_bus_end(GschemToplevel *w_current, int x, int y);
void o_bus_motion(GschemToplevel *w_current, int x, int y);
void o_bus_draw_rubber (GschemToplevel *w_current, EdaRenderer *renderer);
void o_bus_invalidate_rubber(GschemToplevel *w_current);
/* o_circle.c */
void o_circle_invalidate_rubber(GschemToplevel *w_current);
void o_circle_start(GschemToplevel *w_current, int x, int y);
void o_circle_end(GschemToplevel *w_current, int x, int y);
void o_circle_motion(GschemToplevel *w_current, int x, int y);
void o_circle_draw_rubber (GschemToplevel *w_current, EdaRenderer *renderer);
/* o_complex.c */
void o_complex_prepare_place(GschemToplevel *w_current, const CLibSymbol *sym);
void o_complex_place_changed_run_hook(GschemToplevel *w_current);
/* o_copy.c */
void o_copy_start(GschemToplevel *w_current, int x, int y);
void o_copy_end(GschemToplevel *w_current);
/* o_delete.c */
void o_delete(GschemToplevel *w_current, OBJECT *object);
void o_delete_selected(GschemToplevel *w_current, gchar *undo_desc);
/* o_find.c */
gboolean o_find_object(GschemToplevel *w_current, int x, int y,
		       gboolean deselect_afterwards);
gboolean o_find_selected_object(GschemToplevel *w_current, int x, int y);
/* o_grips.c */
OBJECT *o_grips_search_world(GschemToplevel *w_current, int x, int y, int *whichone);
OBJECT *o_grips_search_arc_world(GschemToplevel *w_current, OBJECT *o_current, int x, int y, int size, int *whichone);
OBJECT *o_grips_search_box_world(GschemToplevel *w_current, OBJECT *o_current, int x, int y, int size, int *whichone);
OBJECT *o_grips_search_path_world(GschemToplevel *w_current, OBJECT *o_current, int x, int y, int size, int *whichone);
OBJECT *o_grips_search_picture_world(GschemToplevel *w_current, OBJECT *o_current, int x, int y, int size, int *whichone);
OBJECT *o_grips_search_circle_world(GschemToplevel *w_current, OBJECT *o_current, int x, int y, int size, int *whichone);
OBJECT *o_grips_search_line_world(GschemToplevel *w_current, OBJECT *o_current, int x, int y, int size, int *whichone);
void o_grips_start(GschemToplevel *w_current, int x, int y);
void o_grips_motion(GschemToplevel *w_current, int x, int y);
void o_grips_end(GschemToplevel *w_current);
void o_grips_cancel(GschemToplevel *w_current);
void o_grips_draw_rubber(GschemToplevel *w_current, EdaRenderer *renderer);
/* o_line.c */
void o_line_invalidate_rubber(GschemToplevel *w_current);
void o_line_start(GschemToplevel *w_current, int x, int y);
void o_line_end(GschemToplevel *w_current, int x, int y);
void o_line_motion(GschemToplevel *w_current, int x, int y);
void o_line_draw_rubber(GschemToplevel *w_current, EdaRenderer *renderer);
int o_line_visible(GschemToplevel *w_current, LINE *line, int *x1, int *y1, int *x2, int *y2);
/* o_misc.c */
void o_edit(GschemToplevel *w_current, GList *list, gboolean double_click);
void o_rotate_world_update(GschemToplevel *w_current, int centerx, int centery, int angle, GList *list);
void o_mirror_world_update(GschemToplevel *w_current, int centerx, int centery, GList *list);
void o_edit_show_hidden_lowlevel(GschemToplevel *w_current, const GList *o_list);
void o_edit_show_hidden(GschemToplevel *w_current, const GList *o_list);
void o_edit_hide_specific_text(GschemToplevel *w_current, const GList *o_list, const char *stext);
void o_edit_show_specific_text(GschemToplevel *w_current, const GList *o_list, const char *stext);
OBJECT *o_update_component(GschemToplevel *w_current, OBJECT *o_current);
void o_autosave_backups(GschemToplevel *w_current);
/* o_move.c */
void o_move_start(GschemToplevel *w_current, int x, int y);
void o_move_end_lowlevel(GschemToplevel *w_current, OBJECT *object, int diff_x, int diff_y);
void o_move_end(GschemToplevel *w_current);
void o_move_cancel(GschemToplevel *w_current);
void o_move_motion(GschemToplevel *w_current, int x, int y);
void o_move_invalidate_rubber(GschemToplevel *w_current, int drawing);
void o_move_draw_rubber(GschemToplevel *w_current, EdaRenderer *renderer);
int o_move_return_whichone(OBJECT *object, int x, int y);
void o_move_check_endpoint(GschemToplevel *w_current, OBJECT *object);
void o_move_prep_rubberband(GschemToplevel *w_current);
int o_move_zero_length(OBJECT *object);
void o_move_end_rubberband(GschemToplevel *w_current, int world_diff_x, int world_diff_y, GList **objects);
/* o_net.c */
void o_net_reset(GschemToplevel *w_current);
void o_net_guess_direction(GschemToplevel *w_current, int x, int y);
void o_net_find_magnetic(GschemToplevel *w_current, int event_x, int event_y);
void o_net_finishmagnetic(GschemToplevel *w_current);
void o_net_start_magnetic(GschemToplevel *w_current, int x, int y);
void o_net_start(GschemToplevel *w_current, int x, int y);
void o_net_end(GschemToplevel *w_current, int x, int y);
void o_net_motion(GschemToplevel *w_current, int x, int y);
void o_net_draw_rubber(GschemToplevel *w_current, EdaRenderer *renderer);
void o_net_invalidate_rubber(GschemToplevel *w_current);
int o_net_add_busrippers(GschemToplevel *w_current, OBJECT *net_obj, GList *other_objects);
/* o_ognrst.c */
void o_ognrst_invalidate_rubber (GschemToplevel *w_current);
void o_ognrst_end (GschemToplevel *w_current, int w_x, int w_y);
void o_ognrst_motion (GschemToplevel *w_current, int w_x, int w_y);
void o_ognrst_draw_rubber (GschemToplevel *w_current, EdaRenderer *renderer, int x, int y, int width, int height);
/* o_picture.c */
void o_picture_start(GschemToplevel *w_current, int x, int y);
void o_picture_end(GschemToplevel *w_current, int x, int y);
void o_picture_motion(GschemToplevel *w_current, int x, int y);
void picture_selection_dialog (GschemToplevel *w_current);
void o_picture_invalidate_rubber(GschemToplevel *w_current);
void o_picture_draw_rubber(GschemToplevel *w_current, EdaRenderer *renderer);
gboolean o_picture_exchange(GschemToplevel *w_current, const gchar *filename, GError **error);
void picture_change_filename_dialog (GschemToplevel *w_current);
void o_picture_set_pixbuf(GschemToplevel *w_current, GdkPixbuf *pixbuf, char *filename);

/* o_path.c */
void o_path_start(GschemToplevel *w_current, int x, int y);
void o_path_continue (GschemToplevel *w_current, int w_x, int w_y);
void o_path_motion (GschemToplevel *w_current, int w_x, int w_y);
void o_path_end(GschemToplevel *w_current, int x, int y);
void o_path_end_path (GschemToplevel *w_current);
void o_path_invalidate_rubber (GschemToplevel *w_current);
void o_path_draw_rubber (GschemToplevel *w_current, EdaRenderer *renderer);
void o_path_invalidate_rubber_grips (GschemToplevel *w_current);
void o_path_motion_grips (GschemToplevel *w_current, int x, int y);
void o_path_draw_rubber_grips (GschemToplevel *w_current, EdaRenderer *renderer);

/* o_pin.c */
void o_pin_start(GschemToplevel *w_current, int x, int y);
void o_pin_end(GschemToplevel *w_current, int x, int y);
void o_pin_motion(GschemToplevel *w_current, int x, int y);
void o_pin_draw_rubber(GschemToplevel *w_current, EdaRenderer *renderer);
void o_pin_invalidate_rubber(GschemToplevel *w_current);
/* o_place.c */
void o_place_start(GschemToplevel *w_current, int x, int y);
void o_place_end(GschemToplevel *w_current, int x, int y, int continue_placing, int select_placed, const char *hook_name, const gchar *undo_desc);
void o_place_motion(GschemToplevel *w_current, int x, int y);
void o_place_invalidate_rubber(GschemToplevel *w_current, int drawing);
void o_place_draw_rubber(GschemToplevel *w_current, EdaRenderer *renderer);
void o_place_rotate(GschemToplevel *w_current);
void o_place_mirror(GschemToplevel *w_current);
/* o_select.c */
void o_select_start(GschemToplevel *w_current, int x, int y);
void o_select_end(GschemToplevel *w_current, int x, int y);
void o_select_motion(GschemToplevel *w_current, int x, int y);
void o_select_run_hooks(GschemToplevel *w_current, OBJECT *o_current, int flag);
void o_select_object(GschemToplevel *w_current, OBJECT *o_current, int type, int count);
void o_select_box_start(GschemToplevel *w_current, int x, int y);
void o_select_box_end(GschemToplevel *w_current, int x, int y);
void o_select_box_motion(GschemToplevel *w_current, int x, int y);
void o_select_box_invalidate_rubber(GschemToplevel *w_current);
void o_select_box_draw_rubber(GschemToplevel *w_current, EdaRenderer *renderer);
void o_select_box_search(GschemToplevel *w_current);
void o_select_connected_nets(GschemToplevel *w_current, OBJECT* o_current);
OBJECT *o_select_return_first_object(GschemToplevel *w_current);
int o_select_selected(GschemToplevel *w_current);
void o_select_unselect_all(GschemToplevel *w_current);
void o_select_visible_unlocked(GschemToplevel *w_current);
void o_select_move_to_place_list(GschemToplevel *w_current);
/* o_slot.c */
void o_slot_start(GschemToplevel *w_current, OBJECT *object);
void o_slot_end(GschemToplevel *w_current, OBJECT *object, const char *string);
/* o_text.c */
int o_text_get_rendered_bounds(void *user_data, OBJECT *object, int *min_x, int *min_y, int *max_x, int *max_y);
void o_text_prepare_place(GschemToplevel *w_current, char *text, int color, int align, int rotate, int size);
void o_text_change(GschemToplevel *w_current, OBJECT *object, char *string, int visibility, int show);
gboolean o_text_toggle_overbar (GschemToplevel *w_current, OBJECT *object);
/* o_undo.c */
void o_undo_init(void);
void o_undo_savestate(GschemToplevel *w_current, PAGE *page, int flag, const gchar *desc);
void o_undo_savestate_old(GschemToplevel *w_current, int flag, const gchar *desc);
char *o_undo_find_prev_filename(UNDO *start);
GList *o_undo_find_prev_object_head(UNDO *start);
void o_undo_callback(GschemToplevel *w_current, PAGE *page, int type);
void o_undo_update_actions(GschemToplevel *w_current, PAGE *page);
void o_undo_cleanup(void);
/* parsecmd.c */
int parse_commandline(int argc, char *argv[]);
/* s_stretch.c */
GList *s_stretch_add(GList *list, OBJECT *object, int whichone);
GList *s_stretch_remove(GList *list, OBJECT *object);
void s_stretch_destroy_all(GList *list);
/* gschem_alignment_combo.c */
GtkWidget* gschem_alignment_combo_new ();
int gschem_alignment_combo_get_align (GtkWidget *widget);
void gschem_alignment_combo_set_align (GtkWidget *widget, int align);
/* x_attribedit.c */
gint option_menu_get_history(GtkOptionMenu *option_menu);
void attrib_edit_dialog_ok(GtkWidget *w, GschemToplevel *w_current);
void attrib_edit_dialog(GschemToplevel *w_current, OBJECT *attr_obj, int flag);
/* x_autonumber.c */
void autonumber_text_dialog(GschemToplevel *w_current);
/* x_basic.c */
void x_basic_warp_cursor(GtkWidget *widget, gint x, gint y);
/* x_clipboard.c */
void x_clipboard_init (GschemToplevel *w_current);
void x_clipboard_finish (GschemToplevel *w_current);
void x_clipboard_update_menus (GschemToplevel *w_current);
gboolean x_clipboard_set (GschemToplevel *w_current, const GList *object_list);
GList *x_clipboard_get (GschemToplevel *w_current);
/* x_color.c */
void x_color_init (void);
void x_color_free (void);
void x_color_allocate (void);
GdkColor *x_get_color(int color);
COLOR *x_color_lookup(int color);
gboolean x_color_display_enabled (int index);
/* x_colorcb.c */
GtkWidget* x_colorcb_new ();
int x_colorcb_get_index (GtkWidget *widget);
void x_colorcb_set_index (GtkWidget *widget, int color_index);
void x_colorcb_update_store (void);
/* x_controlfd.c */
void x_controlfd_parsearg (char *optarg);
void x_controlfd_init (void);
void x_controlfd_free (void);
/* x_dialog.c */
int text_view_calculate_real_tab_width(GtkTextView *textview, int tab_size);
void select_all_text_in_textview(GtkTextView *textview);
void text_input_dialog(GschemToplevel *w_current);
void arc_angle_dialog(GschemToplevel *w_current, OBJECT *arc_object);
void slot_edit_dialog(GschemToplevel *w_current, const char *count, const char *string);
void about_dialog(GschemToplevel *w_current);
void x_dialog_hotkeys(GschemToplevel *w_current);
void x_dialog_raise_all(GschemToplevel *w_current);

void generic_msg_dialog(const char *);
int generic_confirm_dialog(const char *);
char * generic_filesel_dialog(const char *, const char *, gint);

void hide_text_dialog(GschemToplevel *w_current);
void show_text_dialog(GschemToplevel *w_current);
gboolean x_dialog_close_changed_page (GschemToplevel *w_current, PAGE *page);
gboolean x_dialog_close_window (GschemToplevel *w_current);
int x_dialog_validate_attribute(GtkWindow* parent, char *attribute);
gboolean x_dialog_confirm_create (GtkWindow *parent, const gchar *message, const gchar *filename);
/* x_event.c */
gint x_event_expose(GschemPageView *widget, GdkEventExpose *event, GschemToplevel *w_current);
gint x_event_raise_dialog_boxes (GschemPageView *view, GdkEventExpose *event, GschemToplevel *w_current);
gint x_event_button_pressed(GschemPageView *page_view, GdkEventButton *event, GschemToplevel *w_current);
gint x_event_button_released(GschemPageView *page_view, GdkEventButton *event, GschemToplevel *w_current);
gint x_event_motion(GschemPageView *page_view, GdkEventMotion *event, GschemToplevel *w_current);
gboolean x_event_faked_motion (GschemPageView *view, GdkEventKey *event);
gboolean x_event_configure (GschemPageView *page_view, GdkEventConfigure *event, gpointer user_data);
gint x_event_enter(GtkWidget *widget, GdkEventCrossing *event, GschemToplevel *w_current);
gboolean x_event_key(GschemPageView *page_view, GdkEventKey *event, GschemToplevel *w_current);
gint x_event_scroll(GtkWidget *widget, GdkEventScroll *event, GschemToplevel *w_current);
gboolean x_event_get_pointer_position (GschemToplevel *w_current, gboolean snapped, gint *wx, gint *wy);
/* gschem_compselect_dockable.c */
void x_compselect_deselect (GschemToplevel *w_current);
void x_compselect_select_previous_symbol (GschemToplevel *w_current);
/* x_fam.c */
void x_fam_init (void);
void x_fam_free (void);
gpointer x_fam_monitor (const gchar *path, void (*exists_event) (const gchar *path, unsigned int code, gpointer user_data), void (*regular_event) (const gchar *path, unsigned int code, gpointer user_data), gpointer user_data);
void x_fam_unmonitor (gpointer handle);
/* x_fileselect.c */
PAGE *x_fileselect_create (GschemToplevel *w_current, const gchar *dirname, const gchar *basename);
void x_fileselect_open(GschemToplevel *w_current);
gboolean x_fileselect_save(GschemToplevel *w_current);
int x_fileselect_load_backup(void *user_data, GString *message);
/* x_fstylecb.c */
GtkWidget* x_fstylecb_new ();
int x_fstylecb_get_index (GtkWidget *widget);
void x_fstylecb_set_index (GtkWidget *widget, int style);
/* x_grid.c */
void x_grid_draw_region(GschemToplevel *w_current, cairo_t *cr, int x, int y, int width, int height);
int x_grid_query_drawn_spacing(GschemToplevel *w_current);
/* x_grid_size_sb.c */
GtkWidget *x_grid_size_sb_new (GschemToplevel *w_current);
/* x_hierarchy.c */
gboolean x_hierarchy_down_schematic (GschemToplevel *w_current, OBJECT *object);
gboolean x_hierarchy_down_symbol (GschemToplevel *w_current, OBJECT *object);
gboolean x_hierarchy_up (GschemToplevel *w_current);
/* x_highlevel.c */
PAGE *x_highlevel_new_page (GschemToplevel *w_current, const gchar *filename);
PAGE *x_highlevel_open_page (GschemToplevel *w_current, const gchar *filename);
gboolean x_highlevel_open_pages (GschemToplevel *w_current, GSList *filenames, gboolean already_confirmed);
gboolean x_highlevel_save_page (GschemToplevel *w_current, PAGE *page);
gboolean x_highlevel_save_all (GschemToplevel *w_current);
gboolean x_highlevel_revert_page (GschemToplevel *w_current, PAGE *page);
gboolean x_highlevel_close_page (GschemToplevel *w_current, PAGE *page);
/* x_image.c */
void x_image_lowlevel(GschemToplevel *w_current, const char* filename,
		      int desired_width, int desired_height, char *filetype);
void x_image_setup(GschemToplevel *w_current);
GdkPixbuf *x_image_get_pixbuf (GschemToplevel *w_current, int width, int height);
/* x_integerls.c */
GtkListStore* x_integerls_new ();
GtkListStore* x_integerls_new_with_values (const char *value[], int count);
void x_integerls_add_value (GtkListStore *store, const char *value);
int x_integerls_get_value_column ();
/* x_linecapcb.c */
GtkWidget* x_linecapcb_new ();
int x_linecapcb_get_index (GtkWidget *widget);
void x_linecapcb_set_index (GtkWidget *widget, int index);
/* x_linetypecb.c */
GtkWidget* x_linetypecb_new ();
int x_linetypecb_get_index (GtkWidget *widget);
void x_linetypecb_set_index (GtkWidget *widget, int index);
/* x_lowlevel.c */
PAGE *x_lowlevel_new_page (GschemToplevel *w_current, const gchar *filename);
PAGE *x_lowlevel_open_page (GschemToplevel *w_current, const gchar *filename);
gboolean x_lowlevel_save_page (GschemToplevel *w_current, PAGE *page, const gchar *filename);
gboolean x_lowlevel_revert_page (GschemToplevel *w_current, PAGE *page);
void x_lowlevel_close_page (GschemToplevel *w_current, PAGE *page);
/* x_misc.c */
gboolean x_show_uri (GschemToplevel *w_current, const gchar *buf, GError **err);
/* x_menus.c */
void x_menus_create_main_menu(GschemToplevel *w_current);
void x_menus_create_main_popup(GschemToplevel *w_current);
void x_menus_create_submenus(GschemToplevel *w_current);
void x_menus_create_toolbar(GschemToplevel *w_current);
/* gschem_messages_dockable.c */
void x_messages_page_changed (GschemToplevel *w_current);
void x_messages_update (GschemToplevel *w_current);
/* gschem_multiattrib_dockable.c */
void x_multiattrib_update (GschemToplevel *w_current);
void x_multiattrib_edit_attribute (GschemToplevel *w_current, OBJECT *object);
/* x_multimulti.c */
/* gschem_pagesel_dockable.c */
void x_pagesel_update (GschemToplevel *w_current);
/* gschem_patch_dockable.c */
gchar *x_patch_guess_filename (PAGE *page);
void x_patch_import (GschemToplevel *w_current);
void x_patch_do_import (GschemToplevel *w_current, PAGE *page);
/* x_preview.c */
/* x_print.c */
gboolean x_print_export_pdf_page (GschemToplevel *w_current, const gchar *filename);
gboolean x_print_export_pdf (GschemToplevel *w_current, const gchar *filename);
void x_print (GschemToplevel *w_current);
/* x_rc.c */
void x_rc_parse_gschem (TOPLEVEL *toplevel, const gchar *rcfile);
/* x_rotatecb.c */
GtkWidget* gschem_rotation_combo_new ();
int gschem_rotation_combo_get_angle (GtkWidget *widget);
void gschem_rotation_combo_set_angle (GtkWidget *widget, int angle);
/* x_script.c */
void setup_script_selector(GschemToplevel *w_current);
/* x_stroke.c */
void x_stroke_init (void);
void x_stroke_free (void);
void x_stroke_record (GschemToplevel *w_current, gint x, gint y);
gint x_stroke_translate_and_execute (GschemToplevel *w_current);
/* x_window.c */
void x_window_setup (GschemToplevel *w_current);
void x_window_create_drawing(GtkWidget *drawbox, GschemToplevel *w_current);
void x_window_setup_draw_events(GschemToplevel *w_current);
void x_window_update_file_change_notification (GschemToplevel *w_current, PAGE *page);
void x_window_update_patch_change_notification (GschemToplevel *w_current, PAGE *page);
void x_window_create_main(GschemToplevel *w_current);
void x_window_close(GschemToplevel *w_current);
void x_window_close_all(GschemToplevel *w_current);
void x_window_set_current_page (GschemToplevel *w_current, PAGE *page);
void x_window_present (GschemToplevel *w_current);
void x_window_set_default_icon (void);
void x_window_init_icons (void);
GschemToplevel* x_window_new (TOPLEVEL *toplevel);
