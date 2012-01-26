/*
 * propono-vnc-tab.h
 * VNC Tab
 * This file is part of propono
 *
 * Copyright (C) 2009 - Jonh Wendell <wendell@bani.com.br>
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

#ifndef __PROPONO_VNC_TAB_H__
#define __PROPONO_VNC_TAB_H__

#include <propono/propono-tab.h>

G_BEGIN_DECLS

#define PROPONO_TYPE_VNC_TAB              (propono_vnc_tab_get_type())
#define PROPONO_VNC_TAB(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_VNC_TAB, ProponoVncTab))
#define PROPONO_VNC_TAB_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_VNC_TAB, ProponoVncTab const))
#define PROPONO_VNC_TAB_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), PROPONO_TYPE_VNC_TAB, ProponoVncTabClass))
#define PROPONO_IS_VNC_TAB(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), PROPONO_TYPE_VNC_TAB))
#define PROPONO_IS_VNC_TAB_CLASS(klass)...(G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_VNC_TAB))
#define PROPONO_VNC_TAB_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), PROPONO_TYPE_VNC_TAB, ProponoVncTabClass))

typedef struct _ProponoVncTabPrivate ProponoVncTabPrivate;
typedef struct _ProponoVncTab        ProponoVncTab;
typedef struct _ProponoVncTabClass   ProponoVncTabClass;


struct _ProponoVncTab 
{
  ProponoTab tab;
  ProponoVncTabPrivate *priv;
};

struct _ProponoVncTabClass 
{
  ProponoTabClass parent_class;
};

GType		propono_vnc_tab_get_type		(void) G_GNUC_CONST;

GtkWidget *	propono_vnc_tab_new 			(ProponoConnection *conn,
							 ProponoWindow     *window);

void		propono_vnc_tab_send_ctrlaltdel		(ProponoVncTab *tab);
void		propono_vnc_tab_paste_text		(ProponoVncTab *tab,
							 const gchar   *text);

gboolean	propono_vnc_tab_set_scaling		(ProponoVncTab *tab, gboolean active);
gboolean	propono_vnc_tab_get_scaling		(ProponoVncTab *tab);
void		propono_vnc_tab_set_keep_ratio		(ProponoVncTab *tab, gboolean active);
gboolean	propono_vnc_tab_get_keep_ratio		(ProponoVncTab *tab);
void		propono_vnc_tab_set_viewonly		(ProponoVncTab *tab, gboolean active);
gboolean	propono_vnc_tab_get_viewonly		(ProponoVncTab *tab);

gint		propono_vnc_tab_get_original_width	(ProponoVncTab *tab);
gint		propono_vnc_tab_get_original_height	(ProponoVncTab *tab);
void		propono_vnc_tab_original_size		(ProponoVncTab *tab);

gboolean	propono_vnc_tab_is_pointer_grab		(ProponoVncTab *tab);
G_END_DECLS

#endif  /* __PROPONO_VNC_TAB_H__  */
/* vim: set ts=8: */
