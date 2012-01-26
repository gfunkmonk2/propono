/*
 * propono-notebook.h
 * This file is part of propono
 *
 * Copyright (C) 2007,2009 - Jonh Wendell <wendell@bani.com.br>
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
  
#ifndef __PROPONO_NOTEBOOK_H__
#define __PROPONO_NOTEBOOK_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PROPONO_TYPE_NOTEBOOK		(propono_notebook_get_type ())
#define PROPONO_NOTEBOOK(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), PROPONO_TYPE_NOTEBOOK, ProponoNotebook))
#define PROPONO_NOTEBOOK_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), PROPONO_TYPE_NOTEBOOK, ProponoNotebookClass))
#define PROPONO_IS_NOTEBOOK(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), PROPONO_TYPE_NOTEBOOK))
#define PROPONO_IS_NOTEBOOK_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), PROPONO_TYPE_NOTEBOOK))
#define PROPONO_NOTEBOOK_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), PROPONO_TYPE_NOTEBOOK, ProponoNotebookClass))

typedef struct _ProponoNotebookPrivate	ProponoNotebookPrivate;
typedef struct _ProponoNotebookClass	ProponoNotebookClass;
typedef struct _ProponoNotebook		ProponoNotebook;

#include "propono-window.h"
#include "propono-tab.h"

struct _ProponoNotebook
{
  GtkNotebook notebook;
  ProponoNotebookPrivate *priv;
};

struct _ProponoNotebookClass
{
  GtkNotebookClass parent_class;
};

GType			propono_notebook_get_type		(void) G_GNUC_CONST;
ProponoNotebook *	propono_notebook_new			(ProponoWindow *window);

void			propono_notebook_add_tab		(ProponoNotebook *nb,
								 ProponoTab      *tab,
								 gint           position);
void			propono_notebook_close_tab		(ProponoNotebook *nb,
								 ProponoTab      *tab);
void			propono_notebook_close_all_tabs 	(ProponoNotebook *nb);
void			propono_notebook_close_active_tab	(ProponoNotebook *nb);

void			propono_notebook_show_hide_tabs		(ProponoNotebook *nb);

ProponoTab *		propono_notebook_get_active_tab		(ProponoNotebook *nb);
GSList *		propono_notebook_get_tabs		(ProponoNotebook *nb);

G_END_DECLS

#endif /* __PROPONO_NOTEBOOK_H__ */
/* vim: set ts=8: */
