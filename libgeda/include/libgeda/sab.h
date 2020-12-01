#ifndef SAB_H

#define SAB_H

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libguile.h>

#include "struct.h"
#include "prototype.h"
#include <gmodule.h>


typedef enum
{ discard, bypass, exec } sab_actions;

typedef struct
{
  gchar *label;
  GSList *pins;
} pin_set;

typedef struct
{
  OBJECT *src;                  /* The source attribute - used when we release the attributes */
  gchar *context;
  int order;
  sab_actions action;
  void *action_param;
} sab_action;

typedef struct
{
  OBJECT *base;
  GSList *actions;
} sab_action_set;

sab_action_set *SabGetSet (OBJECT * object);
void SabUpdateBypassPin (sab_action_set * set, char *cur_pin, char *new_pin);
void SabReleaseSet (sab_action_set * set, TOPLEVEL * topLevel);

#endif /* SAB_H */
