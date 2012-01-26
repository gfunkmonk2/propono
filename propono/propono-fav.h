/*
 * propono-fav.h
 * This file is part of propono
 *
 * Copyright (C) 2007,2008 - Jonh Wendell <wendell@bani.com.br>
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

#ifndef __PROPONO_FAV_H__
#define __PROPONO_FAV_H__

#include <gtk/gtk.h>
#include "propono-bookmarks-entry.h"
#include "propono-window.h"

G_BEGIN_DECLS

#define PROPONO_TYPE_FAV              (propono_fav_get_type())
#define PROPONO_FAV(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_FAV, ProponoFav))
#define PROPONO_FAV_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_FAV, ProponoFav const))
#define PROPONO_FAV_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), PROPONO_TYPE_FAV, ProponoFavClass))
#define PROPONO_IS_FAV(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), PROPONO_TYPE_FAV))
#define PROPONO_IS_FAV_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_FAV))
#define PROPONO_FAV_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), PROPONO_TYPE_FAV, ProponoFavClass))

typedef struct _ProponoFavPrivate ProponoFavPrivate;
typedef struct _ProponoFav        ProponoFav;
typedef struct _ProponoFavClass   ProponoFavClass;

struct _ProponoFav 
{
  GtkFrame frame;
  ProponoFavPrivate *priv;
};

struct _ProponoFavClass 
{
  GtkFrameClass parent_class;

  /* Signals */
  void	(* fav_activated)   (ProponoFav *fav,
			     ProponoBookmarksEntry *entry);

  void	(* fav_selected)    (ProponoFav *fav,
			    ProponoBookmarksEntry *entry);
};

GType 	    propono_fav_get_type    (void) G_GNUC_CONST;

GtkWidget   *propono_fav_new        (ProponoWindow *window);

gboolean    propono_fav_update_list (ProponoFav *fav);

void        propono_fav_bookmarks_open (GtkAction *action, ProponoFav *fav);

const gchar * propono_fav_get_ui_xml_filename (void);

G_END_DECLS

#endif  /* __PROPONO_FAV_H__  */
