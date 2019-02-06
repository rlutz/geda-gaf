#define DEFINE_ACTION(c_id, id, icon, name, label, menu_label) \
  KEEP_LINE                                                    \
  action_ ## c_id =                                            \
    gschem_action_register (id,                                \
                            icon,                              \
                            name,                              \
                            label,                             \
                            menu_label,                        \
                            i_callback_ ## c_id);
#include "actions.c"
#undef DEFINE_ACTION
