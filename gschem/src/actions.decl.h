#define DEFINE_ACTION(c_id, id, icon, name, label, menu_label) \
  KEEP_LINE GschemAction *action_ ## c_id;
#include "actions.c"
#undef DEFINE_ACTION
