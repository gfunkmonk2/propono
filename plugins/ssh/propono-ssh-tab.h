/*
 * propono-ssh-tab.h
 * SSH Tab
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

#ifndef __PROPONO_SSH_TAB_H__
#define __PROPONO_SSH_TAB_H__

#include <propono/propono-tab.h>

G_BEGIN_DECLS

#define PROPONO_TYPE_SSH_TAB              (propono_ssh_tab_get_type())
#define PROPONO_SSH_TAB(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_SSH_TAB, ProponoSshTab))
#define PROPONO_SSH_TAB_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_SSH_TAB, ProponoSshTab const))
#define PROPONO_SSH_TAB_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), PROPONO_TYPE_SSH_TAB, ProponoSshTabClass))
#define PROPONO_IS_SSH_TAB(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), PROPONO_TYPE_SSH_TAB))
#define PROPONO_IS_SSH_TAB_CLASS(klass)...(G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_SSH_TAB))
#define PROPONO_SSH_TAB_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), PROPONO_TYPE_SSH_TAB, ProponoSshTabClass))

typedef struct _ProponoSshTabPrivate ProponoSshTabPrivate;
typedef struct _ProponoSshTab        ProponoSshTab;
typedef struct _ProponoSshTabClass   ProponoSshTabClass;


struct _ProponoSshTab 
{
  ProponoTab tab;
  ProponoSshTabPrivate *priv;
};

struct _ProponoSshTabClass 
{
  ProponoTabClass parent_class;
};

GType		propono_ssh_tab_get_type		(void) G_GNUC_CONST;

GtkWidget *	propono_ssh_tab_new 			(ProponoConnection *conn,
							 ProponoWindow     *window);

G_END_DECLS

#endif  /* __PROPONO_SSH_TAB_H__  */
/* vim: set ts=8: */
