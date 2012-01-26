/*
 * propono-tab.h
 * Abstract base class for all types of tabs: VNC, RDP, etc.
 * This file is part of propono
 *
 * Copyright (C) 2007,2008,2009 - Jonh Wendell <wendell@bani.com.br>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPONO_TAB_H__
#define __PROPONO_TAB_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PROPONO_TYPE_TAB              (propono_tab_get_type())
#define PROPONO_TAB(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_TAB, ProponoTab))
#define PROPONO_TAB_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_TAB, ProponoTab const))
#define PROPONO_TAB_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), PROPONO_TYPE_TAB, ProponoTabClass))
#define PROPONO_IS_TAB(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), PROPONO_TYPE_TAB))
#define PROPONO_IS_TAB_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_TAB))
#define PROPONO_TAB_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), PROPONO_TYPE_TAB, ProponoTabClass))
#define PROPONO_TAB_KEY               "PROPONO_TAB_KEY"

typedef struct _ProponoTabPrivate ProponoTabPrivate;
typedef struct _ProponoTab        ProponoTab;
typedef struct _ProponoTabClass   ProponoTabClass;

#include "propono-connection.h"
#include "propono-notebook.h"
#include "propono-window.h"

typedef enum
{
  PROPONO_TAB_STATE_INITIALIZING = 1,
  PROPONO_TAB_STATE_CONNECTED,
  PROPONO_TAB_STATE_INVALID
} ProponoTabState;


struct _ProponoTab 
{
  GtkVBox vbox;
  ProponoTabPrivate *priv;
};

typedef struct
{
  gchar     **paths; /* NULL-terminated array of strings */
  GtkAction *action;
} ProponoTabUiAction;

struct _ProponoTabClass 
{
  GtkVBoxClass parent_class;

  /* Signals */
  void		(* tab_connected)			(ProponoTab *tab);
  void		(* tab_disconnected)			(ProponoTab *tab);
  void		(* tab_initialized)			(ProponoTab *tab);
  void		(* tab_auth_failed)			(ProponoTab *tab, const gchar *msg);

  /* Virtual functions */
  void		(* impl_get_dimensions)			(ProponoTab *tab, int *w, int *h);
  const GSList *(* impl_get_always_sensitive_actions)	(ProponoTab *tab);
  const GSList *(* impl_get_connected_actions)		(ProponoTab *tab);
  const GSList *(* impl_get_initialized_actions)	(ProponoTab *tab);
  gchar *	(* impl_get_extra_title)		(ProponoTab *tab);

  /* Abstract functions */
  gchar *	(* impl_get_tooltip)			(ProponoTab *tab);
  GdkPixbuf *	(* impl_get_screenshot)			(ProponoTab *tab);
};

GType			propono_tab_get_type		(void) G_GNUC_CONST;

GtkWidget *		propono_tab_new 		(ProponoConnection *conn,
							 ProponoWindow     *window);

ProponoConnection *	propono_tab_get_conn		(ProponoTab *tab);
ProponoWindow *		propono_tab_get_window		(ProponoTab *tab);

void			propono_tab_add_view		(ProponoTab *tab, GtkWidget *view);
GtkWidget *		propono_tab_get_view		(ProponoTab *tab);

void			propono_tab_set_title		(ProponoTab *tab,
							 const char *title);

void			propono_tab_set_notebook	(ProponoTab *tab,
							 ProponoNotebook *nb);
ProponoNotebook *	propono_tab_get_notebook	(ProponoTab *tab);

ProponoTabState		propono_tab_get_state		(ProponoTab *tab);
ProponoTab *		propono_tab_get_from_connection	(ProponoConnection *conn);

void			propono_tab_take_screenshot	(ProponoTab *tab);
gchar *			propono_tab_get_tooltip		(ProponoTab *tab);
void			propono_tab_get_dimensions	(ProponoTab *tab, int *w, int *h);

const GSList *		propono_tab_get_always_sensitive_actions(ProponoTab *tab);
const GSList *		propono_tab_get_connected_actions	(ProponoTab *tab);
const GSList *		propono_tab_get_initialized_actions	(ProponoTab *tab);
GtkActionGroup *	propono_tab_get_action_group		(ProponoTab *tab);

gchar *			propono_tab_get_extra_title	(ProponoTab *tab);
GtkWidget *		propono_tab_get_toolbar		(ProponoTab *tab);

void			propono_tab_free_actions	(GSList *actions);
const gchar		*propono_tab_get_icon_name	(ProponoTab *tab);

/* Protected functions */
void			propono_tab_set_save_credentials	(ProponoTab *tab, gboolean value);
void			propono_tab_save_credentials_in_keyring (ProponoTab *tab);
gboolean		propono_tab_find_credentials_in_keyring	(ProponoTab *tab,
								 gchar **username,
								 gchar **password);
void			propono_tab_remove_credentials_from_keyring (ProponoTab *tab);

void			propono_tab_remove_from_notebook	(ProponoTab *tab);
void			propono_tab_add_recent_used		(ProponoTab *tab);
void			propono_tab_set_state			(ProponoTab *tab,
								 ProponoTabState state);

void			propono_tab_add_actions			(ProponoTab *tab,
								 const GtkActionEntry *entries,
								 guint n_entries);
void			propono_tab_add_toggle_actions		(ProponoTab *tab,
								 const GtkToggleActionEntry *entries,
								 guint n_entries);
G_END_DECLS

#endif  /* __PROPONO_TAB_H__  */
/* vim: set ts=8: */
