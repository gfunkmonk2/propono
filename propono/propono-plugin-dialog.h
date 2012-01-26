/*
 * propono-plugins-dialog.h
 * This file is part of propono
 *
 * Copyright (C) 2009 Jorge Pereira <jorge@jorgepereira.com.br>
 * 
 * propono-plugins-dialog.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-plugins-dialog.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPONO_PLUGIN_DIALOG_H__
#define __PROPONO_PLUGIN_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PROPONO_TYPE_PLUGIN_DIALOG              (propono_plugin_dialog_get_type())
#define PROPONO_PLUGIN_DIALOG(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_PLUGIN_DIALOG, ProponoPluginDialog))
#define PROPONO_PLUGIN_DIALOG_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_PLUGIN_DIALOG, ProponoPluginDialog const))
#define PROPONO_PLUGIN_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),  PROPONO_TYPE_PLUGIN_DIALOG, ProponoPluginDialogClass))
#define PROPONO_IS_PLUGIN_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), PROPONO_TYPE_PLUGIN_DIALOG))
#define PROPONO_IS_PLUGIN_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_PLUGIN_DIALOG))
#define PROPONO_PLUGIN_DIALOG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj),  PROPONO_TYPE_PLUGIN_DIALOG, ProponoPluginDialogClass))

typedef struct _ProponoPluginDialogPrivate ProponoPluginDialogPrivate;
typedef struct _ProponoPluginDialog ProponoPluginDialog;
typedef struct _ProponoPluginDialogClass ProponoPluginDialogClass;

struct _ProponoPluginDialog 
{
  GtkDialog dialog;
  ProponoPluginDialogPrivate *priv;
};

struct _ProponoPluginDialogClass 
{
  GtkDialogClass parent_class;
};

GType	propono_plugin_dialog_get_type	(void) G_GNUC_CONST;

void	propono_plugin_dialog_show	(GtkWindow *parent);

G_END_DECLS

#endif /* __PROPONO_PLUGIN_DIALOG_H__ */
/* vim: set ts=8: */
